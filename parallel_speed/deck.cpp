#include "deck.h"
#include "random.h"
#include <stdio.h>

void deck::initialize_suit(suit s, int &i, int deck)
{
	card[i].deck = deck;	card[i].rank = ace;      card[i++].suit = s;
	card[i].deck = deck;	card[i].rank = deuce;    card[i++].suit = s;
	card[i].deck = deck;	card[i].rank = trey;     card[i++].suit = s;
	card[i].deck = deck;	card[i].rank = four;     card[i++].suit = s;
	card[i].deck = deck;	card[i].rank = five;     card[i++].suit = s;
	card[i].deck = deck;	card[i].rank = six;      card[i++].suit = s;
	card[i].deck = deck;	card[i].rank = seven;    card[i++].suit = s;
	card[i].deck = deck;	card[i].rank = eight;    card[i++].suit = s;
	card[i].deck = deck;	card[i].rank = nine;     card[i++].suit = s;
	card[i].deck = deck;	card[i].rank = ten;      card[i++].suit = s;
	card[i].deck = deck;	card[i].rank = jack;     card[i++].suit = s;
	card[i].deck = deck;	card[i].rank = queen;    card[i++].suit = s;
	card[i].deck = deck;	card[i].rank = king;     card[i++].suit = s;
}

deck::deck_class(void)
{
	int i = 0;
	int deck;

	for (deck = 0; deck <DECKS; deck++) {
		initialize_suit(spades,   i, deck);
		initialize_suit(hearts,   i, deck);
		initialize_suit(diamonds, i, deck);
		initialize_suit(clubs,    i, deck);
	}
}

void deck::swap_cards(card_t &c1, card_t &c2)
{
	card_t temp;

	temp = c1;
	c1 = c2;
	c2 = temp;
}

void deck::shuffle(void)
{
	int i;
	int a;
	int b;

	for (i=0; i<CARDS*4; i++) {
		a = random(CARDS);
		b = random(CARDS);
		if (a<0) printf("a is too small \n");
		if (b<0) printf("b is too small \n");
		if (a>=CARDS) printf("a is too big %d \n", a);
		if (b>=CARDS) printf("b is too big %d \n", b);
		swap_cards(card[a], card[b]);
	}
}
