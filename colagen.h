#ifndef COLAGEN_H
#define COLAGEN_H

// "ColaGen" cola FIFO (ENCOLAR/DESENCOLAR)
// con elementos de tipo genérico "Item"



template <class Item>
class ColaGen {
  protected:
  	struct Elem {
  		Elem *ant, *sig;
  		int    ocup;
  		Item    e;
  	};
  	Elem *prim, *ulti;
  	int   n;
  public:
    	 ColaGen ();
    void ENCOLAR (Item); // lo pone el último
    Item DESENCOLAR ();  // saca el primero
    int  LONG_COLA ();   // número de elementos que están esperando
    //  ~ColaGen () debería vaciar cola uno a uno y liberar memoria?
};

// uso:
// ColaGen<mi_tipo> Q; declara una nueva cola de elementos de tipo mi_tipo

template <class Item>
ColaGen<Item>::ColaGen () {
	// inicia una nueva cola vacía
    prim = new (nothrow) Elem;
    if (prim == 0)
		cout << "No más colas!!\n";
    else {
	    ulti = prim;
	    prim->sig = prim;
	    prim->ant = prim;
	    prim->ocup = 0;
	    n = 0;
	}
}

template <class Item>
void ColaGen<Item>::ENCOLAR (Item x) {
	// lo pone el último
    if (ulti->sig->ocup == 0) {
		ulti = ulti->sig;
		ulti->ocup = 1;
		ulti->e = x;
	    n++;
    }
	else {
		ulti->sig = new (nothrow) Elem;
		if (ulti->sig == 0)
			cout << "Cola llena!!\n";
    	else {
			ulti->sig->ant = ulti;
			ulti= ulti->sig;
			ulti->sig = prim;
			prim->ant = ulti;
			ulti->ocup = 1;
			ulti->e = x;
			n++;
		}
	}
}

template <class Item>
Item ColaGen<Item>::DESENCOLAR () {
	// saca el primero
	Item x;
	if (n > 0) {
		x = prim->e;
		prim->ocup = 0;
		prim = prim->sig;
		n--;
		return x;
	}
	else
		cout << "Cola vacía!!\n";
}

template <class Item>
int ColaGen<Item>::LONG_COLA () {
  	return n;
}

#endif
