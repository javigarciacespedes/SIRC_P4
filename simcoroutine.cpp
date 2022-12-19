#include "simcoroutine.h"

#include <iostream>
#include <stdlib.h>
#include <string.h>
using namespace std;

// -----------------------------------------------------------

static void Error(const char *Msg) {
    cerr << "Error: " << Msg << endl;
    exit(0);
}

class SQS_Process : public Process {
public:
    SQS_Process() { EVTIME = -1; PRED = SUC = this; }
    void Actions() {}
    inline static void SCHEDULE(Process *Q, Process *P) {
        Q->PRED = P; Q->SUC = P->SUC;
        P->SUC = Q; Q->SUC->PRED = Q;
    }
    inline static void UNSCHEDULE(Process *P) {
        P->PRED->SUC = P->SUC;
        P->SUC->PRED =P->PRED;
        P->PRED = P->SUC = 0;
    }
} SQS;

class Main_Program : public Process {
public:
    Main_Program() { EVTIME = 0; SQS.SCHEDULE(this,&SQS); }
    void Actions() { while (1) Detach(); }
} MainProgram;

Process::Process(size_t stack_size) : Coroutine(stack_size)
    { WAITING = 0; 											//ee
      TERMINATED = 0; PRED = SUC = 0; }
int Process::Waiting() const { return WAITING; }			//ee
void Process::SetWaiting(int w) { WAITING=w; }				//ee

void Process::Routine() {
    Actions();
    TERMINATED = 1;
    WAITING = 2;											//ee
    SQS.UNSCHEDULE(this);
    if (SQS.SUC == &SQS)
        Error("SQS is empty");
    ToBeResumed = SQS.SUC;
}
int Process::Idle() const { return SUC == 0; }
int Process::Terminated() const { return TERMINATED; }
double Process::EvTime() const {
    if (SUC == 0)
        Error("No EvTime for Idle Process");
    return EVTIME;
}
Process* Process::NextEv() const { return SUC == &SQS ? 0 : SUC; }

Process *Main() { return &MainProgram; }
Process *Current() { return SQS.SUC; }
double Time() { return SQS.SUC->EVTIME; }

void Hold(double T) {
    Process *Q = SQS.SUC;
    if (T > 0)
        Q->EVTIME += T;
    T = Q->EVTIME;
    if (Q->SUC != &SQS && Q->SUC->EVTIME <= T) {
        SQS.UNSCHEDULE(Q);
        Process *P = SQS.PRED;
        while (P->EVTIME > T)
            P = P->PRED;
        SQS.SCHEDULE(Q,P);
        Resume(SQS.SUC);
    }
}

void Passivate() {
    Process *CURRENT = SQS.SUC;
    SQS.UNSCHEDULE(CURRENT);
    if (SQS.SUC == &SQS)
        Error("SQS is empty");
    Resume(SQS.SUC);
}

void Wait(Head *Q) {
    Process *CURRENT = SQS.SUC;
    CURRENT->Into(Q);
    SQS.UNSCHEDULE(CURRENT);
    if (SQS.SUC == &SQS)
        Error("SQS is empty");
    Resume(SQS.SUC);
}

void Cancel(Process *P) {
    if (!P || !P->SUC)
        return;
    Process *CURRENT = SQS.SUC;
    SQS.UNSCHEDULE(P);
    if (SQS.SUC != CURRENT)
        return;
    if (SQS.SUC == &SQS)
        Error("SQS is empty");
    Resume(SQS.SUC);
}

enum {direct = 0};

void Activat(int Reac, Process *X, int Code,
    double T, Process *Y, int Prio) {
    if (!X || X->TERMINATED || (!Reac && X->SUC))
        return;
    Process *CURRENT = SQS.SUC, *P = 0;
    double NOW = CURRENT->EVTIME;
    switch(Code) {
    case direct:
        if (X == CURRENT)
            return;
        T = NOW; P = &SQS;
        break;
    case delay:
        T += NOW;
    case at:
        if (T <= NOW) {
            if (Prio && X == CURRENT)
                return;
            T = NOW;
        }
        break;
    case before:
    case after:
        if (!Y || !Y->SUC) {
            SQS.UNSCHEDULE(X);
            if (SQS.SUC == &SQS)
                Error("SQS is empty");
            return;
        }
        if (X == Y)
            return;
        T = Y->EVTIME;
        P = Code == before ? Y->PRED : Y;
    }
    if (X->SUC)
        SQS.UNSCHEDULE(X);
    if (!P) {
        for (P = SQS.PRED; P->EVTIME > T; P = P->PRED)
            ;
        if (Prio)
            while (P->EVTIME == T)
                P = P->PRED;
    }
    X->EVTIME = T;
    SQS.SCHEDULE(X,P);
    if (SQS.SUC != CURRENT)
        Resume(SQS.SUC);
}

void Activate(Process *P) { Activat(0,P,direct,0,0,0); }
void Activate(Process *P, Haste H, double T) { Activat(0,P,H,T,0,0); }
void Activate(Process *P, Haste H, double T, Prior Pri) { Activat(0,P,H,T,0,Pri); }
void Activate(Process *P1, Ranking Rank, Process *P2) { Activat(0,P1,Rank,0,P2,0); }

void Reactivate(Process *P) { Activat(1,P,direct,0,0,0); }
void Reactivate(Process *P, Haste H, double T) { Activat(1,P,H,T,0,0); }
void Reactivate(Process *P, Haste H, double T, Prior Pri)  { Activat(1,P,H,T,0,Pri); }
void Reactivate(Process *P1, Ranking Rank, Process *P2) { Activat(1,P1,Rank,0,P2,0); }

void Accum(double &A, double &B, double &C, double D) {
    A += C*(Time() - B); B = Time(); C += D;
}

// -----------------------------------------------------------

#define Synchronize // {jmp_buf E; if (!setjmp(E)) longjmp(E,1);}

char *StackBottom;
int NumEnters = 0;

#define Terminated(C) (!(C)->StackBuffer && (C)->BufferSize)

static Coroutine *CCurrent = 0, *Next;

Coroutine *Coroutine::ToBeResumed = 0;

static class MainCoroutine : public Coroutine {
  public:
    MainCoroutine() { CCurrent = this; }
    void Routine() {}
} CMain;

Coroutine::Coroutine(size_t Dummy) {
  char X;
  if (StackBottom)
    if (&X < StackBottom ?
        &X <= (char*) this && (char*) this <= StackBottom :
        &X >= (char*) this && (char*) this >= StackBottom
       )
      Error("Attempt to allocate a Coroutine on the stack");
  StackBuffer = 0; Low = High = 0; BufferSize = Dummy = 0;
  Callee = Caller = 0;
}

Coroutine::~Coroutine() {
  delete StackBuffer;
  StackBuffer = 0;
}

inline void Coroutine::RestoreStack() {
  Synchronize;
  char X;
  if (&X >= Low && &X <= High)
    RestoreStack();
  CCurrent = this;
  memcpy(Low, StackBuffer, High - Low);
  longjmp(CCurrent->Environment, 1);
}

inline void Coroutine::StoreStack() {
  if (!Low) {
    if (!StackBottom)
      Error("StackBottom is not initialized");
    Low = High = StackBottom;
  }
  char X;
  if (&X > StackBottom)
    High = &X;
  else
    Low = &X;
  if (High - Low > BufferSize) {
    delete StackBuffer;
    BufferSize = High - Low;
    if (!(StackBuffer = new char[BufferSize]))
      Error("No more space available");
  }
  Synchronize;
  memcpy(StackBuffer, Low, High - Low);
}

inline void Coroutine::Enter() {
  NumEnters++;
  if (!Terminated(CCurrent)) {
    CCurrent->StoreStack();
    if (setjmp(CCurrent->Environment))
      return;
  }
  CCurrent = this;
  if (!StackBuffer) {
    Routine();
    delete CCurrent->StackBuffer;
    CCurrent->StackBuffer = 0;
    if (ToBeResumed) {
      Next = ToBeResumed;
      ToBeResumed = 0;
      Resume(Next);
    }
    Detach();
  }
  RestoreStack();
}

void Resume(Coroutine *Next) {
  if (!Next)
    Error("Attempt to Resume a non-existing Coroutine");
  if (Next == CCurrent)
    return;
  if (Terminated(Next))
    Error("Attempt to Resume a terminated Coroutine");
  if (Next->Caller)
    Error("Attempt to Resume an attached Coroutine");
  while (Next->Callee)
    Next = Next->Callee;
  Next->Enter();
}

void Call(Coroutine *Next) {
  if (!Next)
    Error("Attempt to Call a non-existing Coroutine");
  if (Terminated(Next))
    Error("Attempt to Call a terminated Coroutine");
  if (Next->Caller)
    Error("Attempt to Call an attached Coroutine");
  CCurrent->Callee = Next;
  Next->Caller = CCurrent;
  while (Next->Callee)
    Next = Next->Callee;
  if (Next == CCurrent)
    Error("Attempt to Call an operating Coroutine");
  Next->Enter();
}

void Detach() {
  Next = CCurrent->Caller;
  if (Next)
    CCurrent->Caller = Next->Callee = 0;
  else {
    Next = &CMain;
    while (Next->Callee)
      Next = Next->Callee;
  }
  Next->Enter();
}

Coroutine *CurrentCoroutine() { return CCurrent; }

Coroutine *MainCoroutine() { return &CMain; }

// -----------------------------------------------------------

Linkage::Linkage() {
  SUC = PRED = 0;
}

Link* Linkage::Pred() const {
  return PRED ? PRED->LINK() : 0;
}

Link* Linkage::Suc() const {
  return SUC ? SUC->LINK() : 0;
}

Linkage* Linkage::Prev() const {
  return PRED;
}

Head::Head() {
  SUC = PRED = this;
}

Link* Head::First() const {
  return Suc();
}

Link* Head::Last() const {
  return Pred();
}

int Head::Cardinal() const {
  int i = 0;
  for (Link *L = First(); L; L = L->Suc())
    i++;
  return i;
}

int Head::Empty() const {
  return SUC == this;
}

void Head::Clear() {
  while (First())
    First()->Out();
}

Link* Head::LINK() {
  return 0;
}

void Link::Out() {
  if (SUC) {
    SUC->PRED = PRED;
    PRED->SUC = SUC;
    PRED = SUC = 0;
  }
}

void Link::Into(Head *H) {
  Precede(H);
}

void Link::Precede(Linkage *L) {
  Out();
  if (L && L->SUC) {
    SUC = L;
    PRED = L->PRED;
    L->PRED = PRED->SUC = this;
  }
}

void Link::Follow(Linkage *L) {
  if (L)
    Precede(L->SUC);
  else
    Out();
}

Link* Link::LINK() {
  return this;
}

// -----------------------------------------------------------

