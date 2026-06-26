#pragma once

#define cu_abs(x) ((x) >= 0 ? (x) : -(x))
#define cu_min(a, b) ((a) < (b) ? (a) : (b))
#define cu_max(a, b) ((a) >= (b) ? (a) : (b))
#define cu_clamp(v, min, max) ((v) < (min) ? (min) : (v) > (max) ? (max) : (v))
#define cu_clamp_bot(v, bottom) (Max(v, bottom))
#define cu_clamp_top(v, top) (Min(v, top))

#define cu_kilobytes(n) (1024 * (n))
#define cu_megabytes(n) (1024 * cu_kilobytes(n))
#define cu_gigabytes(n) (1024 * cu_megabytes(n))

#define cu_dll_remove(f,l,n,next,prev) (((n)==(f) ? (f) = (n)->next : 0),   \
        ((n) == (l) ? (l) = (l)->prev : 0),                             \
        ((n)->prev != NULL ? ((n)->prev->next = (n)->next) : 0),        \
        ((n)->next != NULL ? ((n)->next->prev = (n)->prev) : 0))
#define cu_dll_push_back(f,l,n,next,prev) (((f)==NULL)?\
        ((f)=(l)=(n),(n)->prev=NULL,(n)->next=NULL):\
        ((l)->next=(n),(n)->prev=(l),(l)=(n),(n)->next=NULL))
#define cu_dll_push_front(f,l,n,next, prev) (((f)==NULL)?    \
        ((f)=(l)=(n), (n)->prev=NULL,(n)->next=NULL):\
        ((n)->next=(f),(f)->prev=n,(f)=(n),(n)->prev=NULL))

#define cu_sll_queue_push(f,l,n) (((f)==NULL)?\
        ((f)=(l)=(n),(n)->next=NULL):           \
        ((l)->next=(n),(l)=(n),(n)->next=NULL))
#define cu_sll_queue_pop(f,l,n) (((f)==(l))?         \
        (((f)=NULL, (l)=NULL)):                 \
        ((f)=(f)->next))

#define cu_sll_stack_push(f,n) ((n)->next = (f),(f)=(n))
#define cu_sll_stack_pop(f)    ((f) = (f)->next)

#define cu_local_persist static

#define cu_type_of(x) (decltype(x))
#define cu_size_of(x) (isize)(sizeof(x))
#define cu_count_of(x) (cu_size_of(x) / sizeof(0[x]))

#define CU_DEFER_LOOP(begin, end) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))

#define IsPow2(x)          ((x)!=0 && ((x)&((x)-1))==0)
// #define AlignForward(x, a) ((x)+(a)-((x)&((a)-1)))
#define AlignForward(x,b)     (((x) + (b) - 1)&(~((b) - 1)))
#define AlignDownPow2(x,b) ((x)&(~((b) - 1)))

#define cu_memcopy(dest, src, bytes) (memmove(dest, src, bytes)) 
#define cu_zero_item(dest, bytes)    (memset(dest, 0, bytes))

#define cu_swap(a,b) do{cu_type_of(a) _tmp = (a); (a) = (b); (b) = _tmp;}while(0)
#define cu_quick_sort(Arr,T,N,Func) qsort((Arr), (N), sizeof(T), (Func))

inline Axis flip_axis(Axis axis) {
    if (axis == Axis_X) {
        return Axis_Y;
    } else {
        return Axis_X;
    }
}
