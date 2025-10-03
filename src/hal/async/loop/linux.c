// loop_epoll.c
#include "stealthim/hal/async/loop.h"

#ifdef STEALTHIM_ASYNC_LINUX

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>

// -------------------- 数据结构 --------------------

typedef struct timer_req_s {
    loop_cb_t cb;
    void *userdata;
    timer_id_t id;
    uint64_t expire_ms;
    int cancelled;
} timer_req_t;

typedef struct handle_req_s {
    int fd;
    loop_cb_t cb;           // 用户回调（当收到数据时，回调传入 recv_data_t*）
    void *userdata;         // 用户 userdata，与 Windows 版本语义保持一致
    struct handle_req_s *next;
    char *buf;
    int buf_len;
    int closing;            // 标记正在注销（由任何线程设置）
    struct handle_req_s *next_free; // 延迟释放链表
} handle_req_t;

typedef struct posted_event_s {
    loop_cb_t cb;
    void *userdata;
    struct posted_event_s *next;
} posted_event_t;

struct loop_s {
    int epfd;               // epoll fd
    int wakefd;             // eventfd 用来唤醒 / posted
    volatile long next_timer_id;
    int stop_flag;
    pthread_t thread;
    pthread_mutex_t lock;

    timer_req_t **heap;
    int heap_size;
    int heap_cap;

    handle_req_t *handles;      // 链表（active handles）
    handle_req_t *to_free;      // 由其他线程请求注销的 handle，主循环在安全时释放

    posted_event_t *posted_head;
    posted_event_t *posted_tail;
};

// -------------------- 时间辅助 --------------------

static uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000ULL);
}

uint64_t stealthim_loop_time_ms(loop_t *loop) {
    (void)loop;
    return now_ms();
}

// -------------------- 最小堆操作（须在 mutex 保护下） --------------------

static void heap_swap(timer_req_t **a, timer_req_t **b) {
    timer_req_t *tmp = *a;
    *a = *b;
    *b = tmp;
}

static void heap_up(timer_req_t **heap, int idx) {
    while (idx > 0) {
        int p = (idx - 1) / 2;
        if (heap[p]->expire_ms <= heap[idx]->expire_ms) break;
        heap_swap(&heap[p], &heap[idx]);
        idx = p;
    }
}

static void heap_down(timer_req_t **heap, int n, int idx) {
    while (1) {
        int l = idx * 2 + 1, r = idx * 2 + 2, smallest = idx;
        if (l < n && heap[l]->expire_ms < heap[smallest]->expire_ms) smallest = l;
        if (r < n && heap[r]->expire_ms < heap[smallest]->expire_ms) smallest = r;
        if (smallest == idx) break;
        heap_swap(&heap[idx], &heap[smallest]);
        idx = smallest;
    }
}

static void heap_push(loop_t *loop, timer_req_t *t) {
    if (loop->heap_size == loop->heap_cap) {
        int newcap = loop->heap_cap ? loop->heap_cap * 2 : 16;
        timer_req_t **tmp = realloc(loop->heap, (size_t)newcap * sizeof(timer_req_t *));
        if (!tmp) return; // OOM: 忍痛不插入
        loop->heap = tmp;
        loop->heap_cap = newcap;
    }
    loop->heap[loop->heap_size] = t;
    heap_up(loop->heap, loop->heap_size);
    loop->heap_size++;
}

static timer_req_t *heap_top(loop_t *loop) {
    return loop->heap_size ? loop->heap[0] : NULL;
}

static timer_req_t *heap_pop(loop_t *loop) {
    if (!loop->heap_size) return NULL;
    timer_req_t *ret = loop->heap[0];
    loop->heap_size--;
    if (loop->heap_size > 0) {
        loop->heap[0] = loop->heap[loop->heap_size];
        heap_down(loop->heap, loop->heap_size, 0);
    }
    return ret;
}

// -------------------- posted 事件队列 --------------------

void stealthim_loop_call_soon(loop_t *loop, loop_cb_t cb, void *userdata) {
    if (!loop || !cb) return;
    posted_event_t *p = calloc(1, sizeof(posted_event_t));
    if (!p) return;
    p->cb = cb;
    p->userdata = userdata;
    p->next = NULL;

    pthread_mutex_lock(&loop->lock);
    if (loop->posted_tail) {
        loop->posted_tail->next = p;
        loop->posted_tail = p;
    } else {
        loop->posted_head = loop->posted_tail = p;
    }
    pthread_mutex_unlock(&loop->lock);

    // 唤醒 epoll（写 eventfd）
    uint64_t one = 1;
    ssize_t r = write(loop->wakefd, &one, sizeof(one));
    (void)r;
}

void stealthim_loop_post(loop_t *loop, loop_cb_t cb, void *userdata) {
    // thread-safe wrapper; semantics same为简单实现
    stealthim_loop_call_soon(loop, cb, userdata);
}

// -------------------- handle (socket) 的注册/注销 --------------------

// helper: set non-blocking
static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;
    return 0;
}

int stealthim_loop_register_handle(loop_t *loop, void *handle, loop_cb_t cb, void *userdata) {
    if (!loop || !handle || !cb) return -1;
    int fd = (int)(intptr_t)handle;

    // set non-blocking (required for EPOLLET)
    if (set_nonblocking(fd) != 0) {
        // non-fatal? return error
        return -1;
    }

    handle_req_t *req = calloc(1, sizeof(handle_req_t));
    if (!req) return -1;
    req->fd = fd;
    req->cb = cb;
    req->userdata = userdata;
    req->buf_len = 4096;
    req->buf = malloc((size_t)req->buf_len);
    if (!req->buf) { free(req); return -1; }
    req->closing = 0;
    req->next = NULL;
    req->next_free = NULL;

    // insert to handles list
    pthread_mutex_lock(&loop->lock);
    req->next = loop->handles;
    loop->handles = req;
    pthread_mutex_unlock(&loop->lock);

    // register to epoll
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.ptr = req; // directly store pointer
    ev.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP;
    if (epoll_ctl(loop->epfd, EPOLL_CTL_ADD, fd, &ev) != 0) {
        // rollback
        pthread_mutex_lock(&loop->lock);
        handle_req_t **pp = &loop->handles;
        while (*pp && *pp != req) pp = &(*pp)->next;
        if (*pp) *pp = req->next;
        pthread_mutex_unlock(&loop->lock);

        free(req->buf);
        free(req);
        return -1;
    }

    return 0;
}

int stealthim_loop_unregister_handle(loop_t *loop, void *handle) {
    if (!loop || !handle) return -1;
    int fd = (int)(intptr_t)handle;

    pthread_mutex_lock(&loop->lock);
    handle_req_t **pp = &loop->handles;
    handle_req_t *found = NULL;
    while (*pp) {
        if ((*pp)->fd == fd) {
            found = *pp;
            *pp = (*pp)->next;
            break;
        }
        pp = &(*pp)->next;
    }
    if (!found) {
        pthread_mutex_unlock(&loop->lock);
        return -1;
    }

    // mark closing and add to to_free list (主循环线程会在安全时释放)
    found->closing = 1;
    found->next_free = loop->to_free;
    loop->to_free = found;

    pthread_mutex_unlock(&loop->lock);

    // 从 epoll 中删除并关闭 fd（关闭后 epoll 可能产生事件，但 pointer 仍然有效直到主线程释放）
    epoll_ctl(loop->epfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);

    // 唤醒主循环以尽快回收
    uint64_t one = 1;
    write(loop->wakefd, &one, sizeof(one));

    return 0;
}

// -------------------- 定时器 API --------------------

timer_id_t stealthim_loop_add_timer(loop_t *loop, uint64_t when_ms, loop_cb_t cb, void *userdata) {
    if (!loop || !cb) return -1;
    timer_req_t *req = calloc(1, sizeof(timer_req_t));
    if (!req) return -1;
    req->cb = cb;
    req->userdata = userdata;
    req->id = (timer_id_t)__sync_add_and_fetch(&loop->next_timer_id, 1);
    req->expire_ms = now_ms() + when_ms;
    req->cancelled = 0;

    pthread_mutex_lock(&loop->lock);
    heap_push(loop, req);
    pthread_mutex_unlock(&loop->lock);

    // 唤醒主循环重新计算超时
    uint64_t one = 1;
    write(loop->wakefd, &one, sizeof(one));

    return req->id;
}

int stealthim_loop_cancel_timer(loop_t *loop, timer_id_t id) {
    if (!loop) return -1;
    pthread_mutex_lock(&loop->lock);
    for (int i = 0; i < loop->heap_size; i++) {
        if (loop->heap[i]->id == id) {
            loop->heap[i]->cancelled = 1;
            pthread_mutex_unlock(&loop->lock);
            uint64_t one = 1;
            write(loop->wakefd, &one, sizeof(one));
            return 0;
        }
    }
    pthread_mutex_unlock(&loop->lock);
    return -1;
}

// -------------------- 主循环内部辅助 --------------------

static void process_posted(loop_t *loop) {
    // 取出队列（一次性取出，减少持锁时间）
    posted_event_t *head = NULL;
    pthread_mutex_lock(&loop->lock);
    head = loop->posted_head;
    loop->posted_head = loop->posted_tail = NULL;
    pthread_mutex_unlock(&loop->lock);

    posted_event_t *p = head;
    while (p) {
        posted_event_t *next = p->next;
        if (p->cb) p->cb(loop, p->userdata);
        free(p);
        p = next;
    }
}

static void free_pending_handles(loop_t *loop) {
    // 在主循环里安全地释放 to_free 列表
    pthread_mutex_lock(&loop->lock);
    handle_req_t *p = loop->to_free;
    loop->to_free = NULL;
    pthread_mutex_unlock(&loop->lock);

    while (p) {
        handle_req_t *next = p->next_free;
        // free buffer & struct
        free(p->buf);
        free(p);
        p = next;
    }
}

static void handle_epoll_handle_event(loop_t *loop, handle_req_t *req) {
    if (!req) return;
    if (req->closing) return; // 被标记为关闭，忽略

    // 边缘触发：循环读取直到 EAGAIN
    for (;;) {
        ssize_t r = recv(req->fd, req->buf, (size_t)req->buf_len, 0);
        if (r > 0) {
            // 构造 recv_data_t，注意这里是栈上临时结构，回调在 loop 线程内同步调用，所以安全
            recv_data_t data;
            data.userdata = req->userdata;
            data.data = req->buf;
            data.len = (unsigned long)r;
            if (req->cb) req->cb(loop, &data);
            // 继续循环读取
            continue;
        } else if (r == 0) {
            // peer closed: unregister (this will mark closing and schedule for free)
            stealthim_loop_unregister_handle(loop, (void *)(intptr_t)req->fd);
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // no more data
                break;
            } else {
                // error: unregister
                stealthim_loop_unregister_handle(loop, (void *)(intptr_t)req->fd);
                break;
            }
        }
    }
}

// -------------------- loop 主循环 --------------------

void loop_run_main(loop_t *loop) {
    const int MAX_EVENTS = 32;
    struct epoll_event events[MAX_EVENTS];

    while (!loop->stop_flag) {
        // 计算下一个定时器到期时间
        int timeout_int = -1; // -1 -> infinite
        pthread_mutex_lock(&loop->lock);
        timer_req_t *top = heap_top(loop);
        if (top) {
            uint64_t now = now_ms();
            if (top->expire_ms <= now) {
                // 触发它（pop）
                timer_req_t *t = heap_pop(loop);
                // 如果被取消则丢弃
                if (!t->cancelled) {
                    loop_cb_t cb = t->cb;
                    void *ud = t->userdata;
                    // 在调用回调前释放锁以允许回调操作 loop（避免死锁）
                    pthread_mutex_unlock(&loop->lock);
                    cb(loop, ud);
                    free(t);
                    // loop back to recompute timers
                    continue;
                } else {
                    free(t);
                    pthread_mutex_unlock(&loop->lock);
                    continue;
                }
            } else {
                uint64_t diff = top->expire_ms - now;
                if (diff > (uint64_t)INT_MAX) timeout_int = INT_MAX;
                else timeout_int = (int)diff;
            }
        }
        pthread_mutex_unlock(&loop->lock);

        int n = epoll_wait(loop->epfd, events, MAX_EVENTS, timeout_int);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        } else if (n == 0) {
            // timeout -> next loop iteration will handle timer
            continue;
        }

        for (int i = 0; i < n; i++) {
            void *ptr = events[i].data.ptr;
            if (!ptr) continue;

            // wakefd is registered with data.ptr == loop pointer
            if (ptr == (void *)loop) {
                // 清空 eventfd (可能累计多个)
                uint64_t cnt;
                while (read(loop->wakefd, &cnt, sizeof(cnt)) > 0) { }
                // 处理 posted 事件
                process_posted(loop);
                // 回收待释放的 handles
                free_pending_handles(loop);
            } else {
                handle_req_t *req = (handle_req_t *)ptr;
                // ensure pointer valid; handle_unregister marks closing and moves to to_free but doesn't free immediately,
                // so pointer stays valid here until free_pending_handles runs.
                // dispatch
                handle_epoll_handle_event(loop, req);
                // after handling, try to free pending handles (in case unregister called during callback)
                free_pending_handles(loop);
            }
        }
    }

    // loop stop: cleanup pending frees
    free_pending_handles(loop);
}

// -------------------- loop 控制 --------------------

static void *loop_thread_fn(void *param) {
    loop_run_main((loop_t *)param);
    return NULL;
}

loop_t *stealthim_loop_create() {
    loop_t *loop = calloc(1, sizeof(loop_t));
    if (!loop) return NULL;

    loop->epfd = epoll_create1(0);
    if (loop->epfd < 0) { free(loop); return NULL; }

    loop->wakefd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (loop->wakefd < 0) { close(loop->epfd); free(loop); return NULL; }

    if (pthread_mutex_init(&loop->lock, NULL) != 0) {
        close(loop->wakefd);
        close(loop->epfd);
        free(loop);
        return NULL;
    }

    loop->next_timer_id = 1;
    loop->stop_flag = 0;
    loop->heap = NULL;
    loop->heap_size = loop->heap_cap = 0;
    loop->handles = NULL;
    loop->to_free = NULL;
    loop->posted_head = loop->posted_tail = NULL;

    // 将 wakefd 注册到 epoll。用 loop 指针作为 data.ptr 以便区分
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.ptr = (void *)loop;
    if (epoll_ctl(loop->epfd, EPOLL_CTL_ADD, loop->wakefd, &ev) != 0) {
        pthread_mutex_destroy(&loop->lock);
        close(loop->wakefd);
        close(loop->epfd);
        free(loop);
        return NULL;
    }

    return loop;
}

void stealthim_loop_run(loop_t *loop) {
    if (!loop) return;
    pthread_create(&loop->thread, NULL, loop_thread_fn, loop);
}

void stealthim_loop_stop(loop_t *loop) {
    if (!loop) return;
    loop->stop_flag = 1;
    // 唤醒 epoll_wait
    uint64_t one = 1;
    write(loop->wakefd, &one, sizeof(one));
    if (loop->thread) {
        pthread_join(loop->thread, NULL);
        loop->thread = 0;
    }
}

void stealthim_loop_destroy(loop_t *loop) {
    if (!loop) return;
    stealthim_loop_stop(loop);

    // close epoll & wakefd
    epoll_ctl(loop->epfd, EPOLL_CTL_DEL, loop->wakefd, NULL);
    close(loop->wakefd);
    close(loop->epfd);

    // 清空堆
    pthread_mutex_lock(&loop->lock);
    for (int i = 0; i < loop->heap_size; i++) {
        free(loop->heap[i]);
    }
    free(loop->heap);
    loop->heap = NULL;
    loop->heap_size = loop->heap_cap = 0;

    // 清空 active handles（这些 fd 可能由调用者关闭，但我们也关闭以与 Windows 行为一致）
    handle_req_t *h = loop->handles;
    while (h) {
        handle_req_t *next = h->next;
        // remove from epoll if any
        epoll_ctl(loop->epfd, EPOLL_CTL_DEL, h->fd, NULL);
        close(h->fd);
        free(h->buf);
        free(h);
        h = next;
    }
    loop->handles = NULL;

    // 清空 to_free
    handle_req_t *f = loop->to_free;
    while (f) {
        handle_req_t *next = f->next_free;
        free(f->buf);
        free(f);
        f = next;
    }
    loop->to_free = NULL;

    // 清空 posted
    posted_event_t *p = loop->posted_head;
    while (p) {
        posted_event_t *next = p->next;
        free(p);
        p = next;
    }
    loop->posted_head = loop->posted_tail = NULL;

    pthread_mutex_unlock(&loop->lock);
    pthread_mutex_destroy(&loop->lock);
    free(loop);
}

#endif
