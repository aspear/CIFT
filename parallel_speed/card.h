#ifndef CARD_INCLUDED
#define CARD_INCLUDED

#define DECKS 3
#define PLAYERS (DECKS * 2)
#define PLAY_PILES (PLAYERS)

#define CARDS_IN_HAND 5
#define CARDS_IN_REPLENISH 5
#define CARDS_IN_DRAW 15

#define CARDS (52*DECKS)

enum suit { spades, hearts, diamonds, clubs };

enum rank { ace, deuce, trey, four, five, six, seven, eight, nine, ten, jack, queen, king };

class card_class
{
public:
    int  deck;
    enum rank    rank;
    enum suit    suit;
    char *rank_to_string();
    char *suit_to_string();
    bool operator==(class card_class card);
};

typedef class card_class card_t;

#endif

