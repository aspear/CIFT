#include <stdlib.h>
#include "stack_of_cards.h"

#ifdef LINKED_LIST_IMPLEMENTATION

stack_of_cards::stack_of_cards_class(void)
{
	// constructor
#ifdef WIN32
	stack_lock = CreateMutex(NULL, false, NULL);
#else
	pthread_mutex_init(&stack_lock, NULL);
#endif
	card_list = NULL;
	list_pointer = NULL;
}

stack_of_cards::~stack_of_cards_class(void)
{
	// destructor - free all allocated memory

	card_list_entry *p = card_list, *next;

	while (p) {
		next = p->next;
		free (p);
		p = next;
	}
}

void stack_of_cards::lock(void)
{
#ifdef WIN32
	WaitForSingleObject(stack_lock, INFINITE);
#else
	pthread_mutex_lock(&stack_lock);
#endif
}

void stack_of_cards::unlock(void)
{
#ifdef WIN32
	ReleaseMutex(stack_lock);
#else
	pthread_mutex_unlock(&stack_lock);
#endif
}


card_t *stack_of_cards::first_card(void)
{
	// returns a pointer to the first card in the stack

	list_pointer = card_list;
	return &list_pointer->card;
}

card_t *stack_of_cards::next_card(void)
{
	// returns a pointer to the next card during a scan

	list_pointer = list_pointer->next;
	if (list_pointer) return &list_pointer->card;
	else return NULL;
}

card_t *stack_of_cards::last_card(void)
{
	// returns the last card on the stack

	card_list_entry *p = card_list;

	list_pointer = NULL;
	if (p) {
		while (p->next) p = p->next;
		return &p->card;
	} else {
		return NULL;
	}
}

void stack_of_cards::print(void)
{
	// prints cards in a stack to stdout 

	card_list_entry *p = card_list;

	while (p) {
		printf("  ");
		switch (p->card.rank) {
			case ace      : printf("ace");      break;
			case deuce    : printf("deuce");    break;
			case trey     : printf("trey");     break;
			case four     : printf("four");     break;
			case five     : printf("five");     break;
			case six      : printf("six");      break;
			case seven    : printf("seven");    break;
			case eight    : printf("eight");    break;
			case nine     : printf("nine");     break;
			case ten      : printf("ten");      break;
			case jack     : printf("jack");     break;
			case queen    : printf("queen");    break;
			case king     : printf("king");     break;
			default       : printf("????");     break;
		}
		printf(" of ");
		switch (p->card.suit) {
			case spades   : printf("spades");   break;
			case hearts   : printf("hearts");   break;
			case diamonds : printf("diamonds"); break;
			case clubs    : printf("clubs");    break;
			default       : printf("????");     break;
		}
		printf("\n");
		p = p->next;
	}
}

card_t stack_of_cards::draw(void)
{
	// removes last card from stack, returns that card

	card_list_entry *p = card_list;
	card_t return_card;

	// assert p != NULL

	if (p->next == NULL) {
		return_card = p->card;
		card_list = NULL;
		free(p);
		return return_card;
	}

	while (p->next->next != NULL) p = p->next;

	return_card = p->next->card;
	free(p->next);
	p->next = NULL;
	return return_card;
}

stack_of_cards *stack_of_cards::operator--()
{
	// removes first card from the stack

	card_list_entry *temp;

	if (card_list) {
		temp = card_list;
		card_list = card_list->next;
		free(temp);
	}
	return this;
}


void stack_of_cards::add(card_t card)
{
	card_list_entry *p = card_list;

	if (card_list) {
		while(p->next) p = (p)->next;
		p->next = (card_list_entry *) malloc(sizeof(card_list_entry));
		p = p->next;
		if (p == NULL) { 
			printf("malloc failure, out of memory \n");
			exit(0);
		}
	} else {
		card_list = (card_list_entry *) malloc(sizeof(card_list_entry));
		if (card_list == NULL) {
			printf("malloc failure, out of memory \n");
			exit(0);
		}
		p = card_list;
	}

	p->card = card;
	p->next = NULL;
}

void stack_of_cards::remove(card_t card)
{
	card_list_entry *p = card_list;
	card_list_entry *t;

	if (p->card == card) {
		p = p->next;
		free(card_list);
		card_list = p;
		return;
	}
	
	while (p->next) {
		if (p->next->card == card) {
			t = p->next;
			p->next = p->next->next;
			free(t);
			return;
		}
		p = p->next;
	}

	return; // note, the card was not found, and thus not removed
}

stack_of_cards *stack_of_cards::operator +=(card_t card)
{
	card_list_entry *p = card_list;

	if (card_list) {
		while(p->next) p = (p)->next;
		p->next = (card_list_entry *) malloc(sizeof(card_list_entry));
		p = p->next;
		if (p == NULL) { 
			printf("malloc failure, out of memory \n");
			exit(0);
		}
	} else {
		card_list = (card_list_entry *) malloc(sizeof(card_list_entry));
		if (card_list == NULL) {
			printf("malloc failure, out of memory \n");
			exit(0);
		}
		p = card_list;
	}

	p->card = card;
	p->next = NULL;

	return this;
}

int stack_of_cards::size(void)
{
	int count = 0;
	card_list_entry *p = card_list;

	while (p) {
		count++;
		p = p->next;
	}

	return count;
}

#else // array based implementation

stack_of_cards::stack_of_cards_class(void)
{
	// constructor

#ifdef WIN32
	stack_lock = CreateMutex(NULL, false, NULL);
#else
	pthread_mutex_init(&stack_lock, NULL);
#endif

	number_of_cards = 0;
}

stack_of_cards::~stack_of_cards_class(void)
{

}

void stack_of_cards::lock(void)
{
#ifdef WIN32
	WaitForSingleObject(stack_lock, INFINITE);
#else
	pthread_mutex_lock(&stack_lock);
#endif
}

void stack_of_cards::unlock(void)
{
#ifdef WIN32
	ReleaseMutex(stack_lock);
#else
	pthread_mutex_unlock(&stack_lock);
#endif
}

card_t *stack_of_cards::first_card(void)
{
	// returns a pointer to the first card in the stack

	list_pointer = 0;
	return &card_stack[0];
}

card_t *stack_of_cards::next_card(void)
{
	// returns a pointer to the next card during a scan

	list_pointer++;
	if (list_pointer > 52) printf("List pointer got too large! \n");
	if (list_pointer < number_of_cards) return &card_stack[list_pointer];
	else return NULL;
}

card_t *stack_of_cards::last_card(void)
{
	// returns the last card on the stack

	if (number_of_cards) {
		return &card_stack[number_of_cards-1];
	} else {
		return NULL;
	}
}

void stack_of_cards::print(void)
{
	// prints cards in a stack to stdout 

	int i;

	for (i=0; i<number_of_cards; i++) {
		printf("  %s of %s \n", card_stack[i].rank_to_string(), card_stack[i].suit_to_string());
	}
}

card_t stack_of_cards::draw(void)
{
	// removes last card from stack, returns that card

	// assert number_of_cards > 0

	return card_stack[number_of_cards---1];
}

stack_of_cards *stack_of_cards::operator--()
{
	int i;

	// removes first card from the stack

	for (i=0; i<number_of_cards-1; i++) {
		card_stack[i] = card_stack[i+1];
	}

	number_of_cards--;
	if (number_of_cards<0) printf("Too few cards!! \n");

	return this;
}


void stack_of_cards::add(card_t card)
{
	card_stack[number_of_cards++] = card;
	if (number_of_cards > 52) printf("too many cards!! \n");
}

void stack_of_cards::remove(card_t card)
{
	int i;
	int j;

	for (i=0; i<number_of_cards; i++) {
		if (card_stack[i] == card) {
			for (j=i; j<number_of_cards-1; j++) {
				card_stack[j] = card_stack[j+1];
			}
			number_of_cards--;
			if (number_of_cards < 0) printf("Too few cards!! \n");
			return;
		}
	}
	return; // note, the card was not found, and thus not removed
}

stack_of_cards *stack_of_cards::operator +=(card_t card)
{
	card_stack[number_of_cards++] = card;

	if (number_of_cards > 52) printf("too many cards!! \n");

	return this;
}

int stack_of_cards::size(void)
{
	return number_of_cards;
}


#endif 
