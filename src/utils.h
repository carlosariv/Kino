#pragma once

#define Abs(x) ((x) >= 0 ? (x) : -(x))
#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) >= (b) ? (a) : (b))
#define Clamp(v, min, max) ((v) < (min) ? (min) : (v) > (max) ? (max) : (v))
#define ClampBot(v, bottom) (Max(v, bottom))
#define ClampTop(v, top) (Min(v, top))

#define KB(n) (1024 * (n))
#define MB(n) (1024 * (KB(n)))
#define GB(n) (1024 * (MB(n)))

#define DLLRemove(f,l,n,next,prev) (((n)==(f) ? (f) = (n)->next : 0),   \
        ((n) == (l) ? (l) = (l)->prev : 0),                             \
        ((n)->prev != NULL ? ((n)->prev->next = (n)->next) : 0),        \
        ((n)->next != NULL ? ((n)->next->prev = (n)->prev) : 0))
#define DLLPushBack(f,l,n,next,prev) (((f)==NULL)?\
        ((f)=(l)=(n),(n)->prev=NULL,(n)->next=NULL):\
        ((l)->next=(n),(n)->prev=(l),(l)=(n),(n)->next=NULL))
#define DLLPushFront(f,l,n,next, prev) (((f)==NULL)?    \
        ((f)=(l)=(n), (n)->prev=NULL,(n)->next=NULL):\
        ((n)->next=(f),(f)->prev=n,(f)=(n),(n)->prev=NULL))

#define SLLQueuePush(f,l,n) (((f)==NULL)?\
        ((f)=(l)=(n),(n)->next=NULL):           \
        ((l)->next=(n),(l)=(n),(n)->next=NULL))
#define SLLQueuePop(f,l,n) (((f)==(l))?         \
        (((f)=NULL, (l)=NULL)):                 \
        ((f)=(f)->next))

#define SLLStackPush(f,n) ((n)->next = (f),(f)=(n))
#define SLLStackPop(f)    ((f) = (f)->next)

#define ArrayCount(array) (sizeof((array)) / sizeof((array)[0]))
#define DeferLoop(begin, end) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))

#define IsPow2(x)          ((x)!=0 && ((x)&((x)-1))==0)
// #define AlignForward(x, a) ((x)+(a)-((x)&((a)-1)))
#define AlignForward(x,b)     (((x) + (b) - 1)&(~((b) - 1)))
#define AlignDownPow2(x,b) ((x)&(~((b) - 1)))

#define MemoryCopy(dest, src, bytes) (memmove(dest, src, bytes)) 
#define MemoryZero(dest, bytes)      (memset(dest, 0, bytes))


#define Swap(T,a,b) do{T __t = a; a = b; b = __t;}while(0)
#define quick_sort(Arr,T,N,Func) qsort((Arr), (N), sizeof(T), (Func))
