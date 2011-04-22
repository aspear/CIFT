#include "card.h"
#include <stdlib.h>

bool card_class::operator==(class card_class card)
{
	if ((rank == card.rank) && (suit == card.suit) && (deck == card.deck)) return true;
	else return false;
}

char *card_class::rank_to_string(void)
{
	switch (rank) {
		case ace      : return("ace");     
		case deuce    : return("deuce");   
		case trey     : return("trey");    
		case four     : return("four");    
		case five     : return("five");    
		case six      : return("six");     
		case seven    : return("seven");   
		case eight    : return("eight");   
		case nine     : return("nine");    
		case ten      : return("ten");     
		case jack     : return("jack");    
		case queen    : return("queen");   
		case king     : return("king");    
		default       : return("????");   
	}
	return NULL;
}

char *card_class::suit_to_string(void)
{
	switch (suit) {
		case spades    : return "spades";
		case hearts    : return "hearts";
		case diamonds  : return "diamonds";
		case clubs     : return "clubs";
		default        : return "????";
	}
	return NULL;
}

