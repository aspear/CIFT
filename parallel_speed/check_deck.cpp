#include "check_deck.h"

#include <stdio.h>
#define ANSI            /* Comment out for UNIX version     */
#ifdef ANSI             /* ANSI compatible version          */
#include <stdarg.h>
int average( int first, ... );
#else                   /* UNIX compatible version          */
#include <varargs.h>
int average( va_list );
#endif

bool mark_off_cards(stack_of_cards *s, deck *ref, bool found[])
{
	int i;
	int c;
	card_t *card;

	for (i=0, card=s->first_card(); i<s->size(); i++) {
		for (c=0; c<CARDS; c++) {
			if (*card == ref->card[c]) {
				if (found[c]) {
					printf("Found multiple copies of %s of %s \n", 
						card->rank_to_string(), card->suit_to_string());
					return false;
				} else {
					found[c] = true;
				}
			}
		}
		card = s->next_card();
	}
	return true;
}



bool check_deck(int bogus, ...)
{
	int i;
	va_list ap;
	stack_of_cards *cs;
	int number_of_cards = 0;
	int number_of_stacks = 0;
	int p;
	deck reference_deck;
	bool found[CARDS];

	for (i=0; i<CARDS; i++) {
		found[i] = false;
	}

	va_start(ap, bogus);

	while (cs = va_arg(ap, stack_of_cards *)) {
		for (p=0; p<PLAYERS; p++) {
			number_of_cards += cs[p].size();
			number_of_stacks++;
			if (!mark_off_cards(&(cs[p]), &reference_deck, found)) return false;
		}
	}
    
	va_end(ap);

	if (number_of_cards != CARDS) {
		printf("Deck is bad! \n");
		printf("number of cards: %d \n", number_of_cards);
		printf("number of stacks: %d \n", number_of_stacks);
		for (i=0; i<CARDS; i++) {
			if (!found[i]) {
				printf("The card %s of %s is missing from the deck \n",
					reference_deck.card[i].rank_to_string(), 
					reference_deck.card[i].suit_to_string());
			}
		}
		return false;
	} else {
//		printf("Deck is OK \n");
//		printf("%d cards found in %d stacks \n", number_of_cards, number_of_stacks);
	}

	for (i=0; i<CARDS; i++) {
		if (!found[i]) {
			printf("The card %s of %s is missing from the deck \n",
				reference_deck.card[i].rank_to_string(), 
				reference_deck.card[i].suit_to_string());
			return false;
		}
	}
	return true;
}

