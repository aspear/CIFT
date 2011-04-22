#ifndef DECK_INCLUDED
#define DECK_INCLUDED

#include "card.h"

class deck_class
{
private:
    void initialize_suit(suit s, int &i, int deck);
    void swap_cards(card_t &c1, card_t &c2);
public:
    card_t card[CARDS];
    deck_class(void);
    void shuffle(void);
    void sort(void);
};

typedef class deck_class deck;

#endif

