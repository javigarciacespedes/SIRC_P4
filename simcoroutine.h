#ifndef Simulation
#define Simulation Sequencing

// -----------------------------------------------------------

#define Sequencing(S) {char Dummy; StackBottom = &Dummy; S;}

#include <stddef.h>
#include <setjmp.h>

extern char *StackBottom;

class Coroutine {
  friend void Resume(Coroutine *);
  friend void Call(Coroutine *);
  friend void Detach();
  friend class Process;
  friend unsigned long Choice(long);
  friend void Backtrack();
  protected:
    Coroutine(size_t Dummy = 0);
    ~Coroutine();
    virtual void Routine() = 0;
  private:
    void Enter();
    void StoreStack();
    void RestoreStack();
    char *StackBuffer, *Low, *High;
    size_t BufferSize;
    jmp_buf Environment;
    Coroutine *Caller, *Callee;
    static Coroutine *ToBeResumed;
};

void Resume(Coroutine *);
void Call(Coroutine *);
void Detach();

Coroutine *CurrentCoroutine();
Coroutine *MainCoroutine();

#define DEFAULT_STACK_SIZE 0

// -----------------------------------------------------------

class Link;
class Head;

class Linkage {
  friend class Link;
  friend class Head;
  public:
    Linkage();
    Link *Pred() const;
    Link *Suc() const;
    Linkage *Prev() const;
  private:
    Linkage *PRED, *SUC;
    virtual Link *LINK() = 0;
};

class Head : public Linkage {
  public:
    Head();
    Link *First() const;
    Link *Last() const;
    int Empty(void) const;
    int Cardinal(void) const;
    void Clear(void);
  private:
    Link *LINK();
};

class Link : public Linkage {
  public:
    void Out(void);
    void Into(Head *H);
    void Precede(Linkage *L);
    void Follow(Linkage *L);
  private:
    Link *LINK();
};

// -----------------------------------------------------------

class Process : public Link, public Coroutine {
    friend Process *Current();
    friend double Time();
    friend void Activat(int Reac, Process *X, int Code, double T, Process *Y, int Prio);
    friend void Hold(double T);
    friend void Passivate();
    friend void Wait(Head *Q);
    friend void Cancel(Process *P);
    friend class Main_Program;
    friend class SQS_Process;
  public:
    virtual void Actions() = 0;
    Process(size_t stack_size = DEFAULT_STACK_SIZE);
    int Idle() const;
    int Terminated() const;
    int Waiting() const;	//ee
    void SetWaiting(int w);	//ee
    double EvTime() const;
    Process *NextEv() const;
  private:
    void Routine();
    int TERMINATED;
    int WAITING;			//ee
    Process *PRED, *SUC;
    double EVTIME;
};

Process *Main();
Process *Current();
double Time();

void Hold(double T);
void Passivate();
void Wait(Head *Q);
void Cancel(Process *P);

enum Haste {at = 1, delay = 2};
enum Ranking {before = 3, after = 4};
enum Prior {prior = 5};

void Activate(Process *P);
void Activate(Process *P, Haste H, double T);
void Activate(Process *P, Haste H, double T, Prior Prio);
void Activate(Process *P1, Ranking Rank, Process *P2);

void Reactivate(Process *P);
void Reactivate(Process *P, Haste H, double T);
void Reactivate(Process *P, Haste H, double T, Prior Prio);
void Reactivate(Process *P1, Ranking Rank, Process *P2);

void Accum(double &A, double &B, double &C, double D);

// -----------------------------------------------------------

#endif

