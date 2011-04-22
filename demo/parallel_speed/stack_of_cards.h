#ifndef STACK_OF_CARDS_INCLUDED
#define STACK_OF_CARDS_INCLUDED

#include <stdio.h> 
#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif
#include "card.h"
#define LINKED_LIST_IMPLEMENTATION

#ifdef LINKED_LIST_IMPLEMENTATION

struct card_list_entry_struct {
    card_t card;
    struct card_list_entry_struct *next;
};

typedef struct card_list_entry_struct card_list_entry;

class stack_of_cards_class
{
private:
    card_list_entry *card_list;
    card_list_entry *list_pointer;
#ifdef WIN32
    HANDLE stack_lock;
#else
    pthread_mutex_t stack_lock;
#endif

public:
    stack_of_cards_class(void);
    ~stack_of_cards_class(void);

    stack_of_cards_class * operator+=(const card_t card);
    stack_of_cards_class * operator--();

    void print(void);
    int size(void);

    card_t *first_card(void);
    card_t *last_card(void);
    card_t *next_card(void);
    card_t *previous_card(void);

    void add(const card_t card);
    void remove(const card_t card);
    card_t draw(void);

    void lock(void);
    void unlock(void);
};

#else // array based implementation

class stack_of_cards_class
{
private:
    int number_of_cards;
    int list_pointer;
    card_t card_stack[52];

#ifdef WIN32
    HANDLE stack_lock;
#else
    pthread_mutex_t stack_lock;
#endif

public:
    stack_of_cards_class(void);
    ~stack_of_cards_class(void);

    stack_of_cards_class * operator+=(const card_t card);
    stack_of_cards_class * operator--();

    void print(void);
    int size(void);

    card_t *first_card(void);
    card_t *last_card(void);
    card_t *next_card(void);
    card_t *previous_card(void);

    void add(const card_t card);
    void remove(const card_t card);
    card_t draw(void);

    void lock(void);
    void unlock(void);
};

#endif

typedef class stack_of_cards_class stack_of_cards; 

#endif
