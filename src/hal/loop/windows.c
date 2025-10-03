#include "stealthim/hal/loop.h"

#ifdef STEALTHIM_ASYNC_WIN32

#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


#pragma comment(lib, "ws2_32.lib")

typedef struct timer_req_s {
    loop_cb_t cb;
    void *userdata;
    timer_id_t id;
    loop_t *loop;
    HANDLE hTimer;
    struct timer_req_s *next;
} timer_req_t;

typedef struct handle_req_s {
    SOCKET sock;
    struct handle_req_s *next;
    char *buf;
    int buf_len;
} handle_req_t;

struct loop_s {
    HANDLE iocp;
    HANDLE timer_queue;
    volatile LONG next_timer_id;
    int stop_flag;
    HANDLE thread;
    CRITICAL_SECTION lock;
    timer_req_t *timers;
    handle_req_t *handles;
};

typedef struct {
    loop_cb_t cb;
    void *userdata;
} posted_event_t;

typedef enum { EV_TIMER, EV_POST, EV_SOCKET } loop_ev_type_t;

typedef struct loop_event_s {
    OVERLAPPED ov;             // 用于 IOCP
    loop_ev_type_t type;       // 事件类型
    void *userdata;            // 回调要用的 userdata
    union {
        struct {
            loop_cb_t cb;
        } timer;
        struct {
            loop_cb_t cb;
        } post;
        struct {
            loop_cb_t cb;
            handle_req_t *handle;
        } sock;
    };
} loop_event_t;

// -------------------- 定时器 --------------------

static VOID CALLBACK timer_callback(PVOID lpParam, BOOLEAN TimerOrWaitFired) {
    timer_req_t *req = (timer_req_t*)lpParam;

    loop_event_t *ev = (loop_event_t*)calloc(1, sizeof(loop_event_t));
    ev->type = EV_TIMER;
    ev->userdata = req->userdata;
    ev->timer.cb = req->cb;

    PostQueuedCompletionStatus(req->loop->iocp, 0, (ULONG_PTR)req->hTimer, &ev->ov);
}

timer_id_t loop_add_timer(loop_t *loop, uint64_t when_ms, loop_cb_t cb, void *userdata) {
    timer_req_t *req = (timer_req_t*)calloc(1, sizeof(timer_req_t));
    req->cb = cb;
    req->userdata = userdata;
    req->loop = loop;
    req->id = InterlockedIncrement(&loop->next_timer_id);

    if (!CreateTimerQueueTimer(&req->hTimer, loop->timer_queue,
        timer_callback, req, (DWORD)when_ms, 0, WT_EXECUTEDEFAULT)) {
        free(req);
        return -1;
    }

    EnterCriticalSection(&loop->lock);
    req->next = loop->timers;
    loop->timers = req;
    LeaveCriticalSection(&loop->lock);

    return req->id;
}

int loop_cancel_timer(loop_t *loop, timer_id_t id) {
    EnterCriticalSection(&loop->lock);
    timer_req_t **p = &loop->timers;
    while (*p && (*p)->id != id) p = &(*p)->next;
    if (*p) {
        timer_req_t *req = *p;
        *p = req->next;
        LeaveCriticalSection(&loop->lock);

        DeleteTimerQueueTimer(loop->timer_queue, req->hTimer, NULL);
        free(req);
        return 0;
    }
    LeaveCriticalSection(&loop->lock);
    return -1;
}

// -------------------- posted 事件 --------------------

void loop_call_soon(loop_t *loop, loop_cb_t cb, void *userdata) {
    loop_event_t *ev = (loop_event_t*)calloc(1, sizeof(loop_event_t));
    ev->type = EV_POST;
    ev->userdata = userdata;
    ev->post.cb = cb;

    PostQueuedCompletionStatus(loop->iocp, 0, 0, &ev->ov);
}

void loop_post(loop_t *loop, loop_cb_t cb, void *userdata) {
    loop_call_soon(loop, cb, userdata);
}

// -------------------- socket HANDLE --------------------

int loop_register_handle(loop_t *loop, void *handle, loop_cb_t cb, void *userdata) {
    SOCKET s = (SOCKET)handle;

    if (!CreateIoCompletionPort((HANDLE)s, loop->iocp, (ULONG_PTR)s, 0))
        return -1;

    handle_req_t *req = (handle_req_t*)calloc(1, sizeof(handle_req_t));
    req->sock = s;
    req->buf_len = 1024;
    req->buf = (char*)malloc(req->buf_len);

    EnterCriticalSection(&loop->lock);
    req->next = loop->handles;
    loop->handles = req;
    LeaveCriticalSection(&loop->lock);

    loop_event_t *ev = (loop_event_t*)calloc(1, sizeof(loop_event_t));
    ev->type = EV_SOCKET;
    ev->userdata = userdata;
    ev->sock.cb = cb;
    ev->sock.handle = req;

    DWORD flags = 0;
    if (
        WSARecv(s,
            &(WSABUF){.buf=req->buf,.len=req->buf_len},
            1,
            NULL,
            &flags,
            &ev->ov,
            NULL
            ) == SOCKET_ERROR
    ) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            free(ev);
            free(req->buf);
            free(req);
            return -1;
        }
    }

    return 0;
}

int loop_unregister_handle(loop_t *loop, void *handle) {
    SOCKET s = (SOCKET)handle;
    EnterCriticalSection(&loop->lock);
    handle_req_t **p = &loop->handles;
    while (*p && (*p)->sock != s) p = &(*p)->next;
    if (*p) {
        handle_req_t *req = *p;
        *p = req->next;
        LeaveCriticalSection(&loop->lock);

        CancelIoEx((HANDLE)s, NULL);
        closesocket(s);
        free(req->buf);
        free(req);
        return 0;
    }
    LeaveCriticalSection(&loop->lock);
    return -1;
}

// -------------------- loop 主循环 --------------------

void loop_run_main(loop_t *loop) {
    DWORD bytes;
    ULONG_PTR key;
    OVERLAPPED *ov;

    while (!loop->stop_flag) {
        BOOL ok = GetQueuedCompletionStatus(loop->iocp, &bytes, &key, &ov, INFINITE);
        if (!ok && ov == NULL) continue;
        if (!ov) continue;

        loop_event_t *ev = (loop_event_t*)ov;

        switch (ev->type) {
        case EV_TIMER:
            ev->timer.cb(loop, ev->userdata);
            free(ev);
            break;
        case EV_POST:
            ev->post.cb(loop, ev->userdata);
            free(ev);
            break;
        case EV_SOCKET:
            recv_data_t data = {
                .userdata = ev->userdata,
                .data = ev->sock.handle->buf,
                .len = bytes
            };
            ev->sock.cb(loop, &data);

            // 重新发起 WSARecv
            DWORD flags = 0;
            WSABUF wbuf = {.buf=ev->sock.handle->buf, .len=ev->sock.handle->buf_len};
            WSARecv(
                key,
                &(WSABUF){.buf=ev->sock.handle->buf, .len=ev->sock.handle->buf_len},
                1,
                NULL,
                &flags,
                &ev->ov,
                NULL
                );
            break;
        }
    }
}

// -------------------- loop 控制 --------------------

static DWORD WINAPI loop_thread_fn(LPVOID param) {
    loop_run_main((loop_t*)param);
    return 0;
}

loop_t *loop_create(const char *backend_name) {
    (void)backend_name;
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    loop_t *loop = (loop_t*)calloc(1, sizeof(loop_t));
    loop->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    loop->timer_queue = CreateTimerQueue();
    loop->next_timer_id = 1;
    InitializeCriticalSection(&loop->lock);
    return loop;
}

void loop_run(loop_t *loop) {
    loop->thread = CreateThread(NULL, 0, loop_thread_fn, loop, 0, NULL);
}

void loop_stop(loop_t *loop) {
    loop->stop_flag = 1;
    PostQueuedCompletionStatus(loop->iocp, 0, 0, NULL);
    if (loop->thread) {
        WaitForSingleObject(loop->thread, INFINITE);
        CloseHandle(loop->thread);
        loop->thread = NULL;
    }
}

void loop_destroy(loop_t *loop) {
    if (!loop) return;
    loop_stop(loop);

    DeleteTimerQueueEx(loop->timer_queue, INVALID_HANDLE_VALUE);
    CloseHandle(loop->iocp);

    EnterCriticalSection(&loop->lock);
    timer_req_t *t = loop->timers;
    while (t) { timer_req_t *next = t->next; free(t); t = next; }
    handle_req_t *h = loop->handles;
    while (h) { handle_req_t *next = h->next; closesocket(h->sock); free(h->buf); free(h); h = next; }
    LeaveCriticalSection(&loop->lock);

    DeleteCriticalSection(&loop->lock);
    WSACleanup();
    free(loop);
}

uint64_t loop_time_ms(loop_t *loop) {
    (void)loop;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    return li.QuadPart / 10000ULL;
}

#endif

