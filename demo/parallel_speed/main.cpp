#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#include <process.h>
#else 
#include <pthread.h>
#include <unistd.h>
#include <sched.h> 
#endif

#ifdef WIN32
#define relinquish_exec Sleep(0)
#else 
#define relinquish_exec pthread_yield()
#endif

#include <time.h>

#include "card.h"
#include "deck.h"
#include "stack_of_cards.h"
#include "check_deck.h"
#include "random.h"

#define _CRT_SECURE_NO_WARNINGS

enum game_result { i_win, i_lose, draw };
enum deal_status { good_deal, bad_deal };
enum player_state { unlaunched, wait_for_start, active, stuck, done };

struct player_arg_list_struct {
	stack_of_cards *my_hand;
	stack_of_cards *my_draw_pile;
	stack_of_cards *play_pile;
	enum player_state *my_state;
	bool *player_ready;
	bool *start_game;
	bool *game_over;
	int player_no;
};


struct referee_arg_list_struct {
	enum player_state *player_state_array;
	stack_of_cards *replenish_pile;
	stack_of_cards *player_hand;
	stack_of_cards *play_pile;
	bool *player_ready;
	bool *game_start_flag;
	bool *game_over;
};


bool adjacent(card_t *card1, card_t *card2)
{
	switch (card1->rank) {
		case ace   : 
			if (card2->rank == king) return true;
			if (card2->rank == deuce) return true;
			break;
		case deuce :
			if (card2->rank == ace) return true;
			if (card2->rank == trey) return true;
			break;
		case trey  :
			if (card2->rank == deuce) return true;
			if (card2->rank == four) return true;
			break;
		case four  :
			if (card2->rank == trey) return true;
			if (card2->rank == five) return true;
			break;
		case five  :
			if (card2->rank == four) return true;
			if (card2->rank == six) return true;
			break;
		case six   :
			if (card2->rank == five) return true;
			if (card2->rank == seven) return true;
			break;
		case seven :
			if (card2->rank == six) return true;
			if (card2->rank == eight) return true;
			break;
		case eight :
			if (card2->rank == seven) return true;
			if (card2->rank == nine) return true;
			break;
		case nine  :
			if (card2->rank == eight) return true;
			if (card2->rank == ten) return true;
			break;
		case ten   :
			if (card2->rank == nine) return true;
			if (card2->rank == jack) return true;
			break;
		case jack  :
			if (card2->rank == ten) return true;
			if (card2->rank == queen) return true;
			break;
		case queen :
			if (card2->rank == jack) return true;
			if (card2->rank == king) return true;
			break;
		case king  : 
			if (card2->rank == queen) return true;
			if (card2->rank == ace) return true;
			break;
		default    :
			break;
	}
	return false;
}


bool play(stack_of_cards *player_hand, 
		  stack_of_cards *draw_pile, 
		  stack_of_cards *play_pile, 
		  int player)
{
	int s;
	card_t *current_card = player_hand->first_card();

	while (current_card) {
		for (s=0; s<PLAY_PILES; s++) {
			play_pile[s].lock();
			if (adjacent(current_card, play_pile[s].last_card())) {
				play_pile[s].add(*current_card);
				player_hand->remove(*current_card);
				play_pile[s].unlock();
				if (draw_pile->size() > 0) player_hand->add(draw_pile->draw());
				return true;
			}
			play_pile[s].unlock();
		}
		current_card = player_hand->next_card();
	}
	
	// could not find a card to play
	return false;
}

#ifdef WIN32
void threaded_player(void *argument_list)
#else
void *threaded_player(void *argument_list)
#endif
{
	struct player_arg_list_struct *p = (struct player_arg_list_struct *) argument_list;

	int r;
	int one = 1;

	*(p->my_state) = wait_for_start;
	*(p->player_ready) = true;

	//sched_setaffinity(0, 4, (cpu_set_t *)&p->player_no); 
	//sched_setaffinity(0, 4, (cpu_set_t *)&one); 

	while (!p->start_game);  // wait here until the game is ready to start
	                         // this bit gets set by the referee
	*p->my_state = active;

	while (p->my_hand->size() > 0) {
		r = play(p->my_hand, p->my_draw_pile, p->play_pile, p->player_no);

		if (r) {
			*p->my_state = active;
		} else {
			*p->my_state = stuck;
			relinquish_exec;
		}

		if (*p->game_over) {
			*p->my_state = done;
#ifdef WIN32
			return;
#else
			return 0;
#endif
		}
	}
	*p->my_state = done;
#ifdef WIN32
	return;
#else
	return 0;
#endif
}

int count_ready_players(bool player_ready[])
{
	int p;
	int players_ready = 0;

	for (p=0; p<PLAYERS; p++) {
		if (player_ready[p]) players_ready++;
	}
	return players_ready;
}


int count_players(enum player_state counted_state, enum player_state player_state[])
{
	int p;
	int count = 0;

	for (p=0; p<PLAYERS; p++) {
		if (player_state[p] == counted_state) count++;
	}
	return count;
}


void turn_cards(stack_of_cards play_pile[], stack_of_cards replenish_pile[])
{
	int p;

	for (p=0; p<PLAYERS; p++) {
		play_pile[p].lock();
	}

	for (p=0; p<PLAYERS; p++) {
		play_pile[p] += replenish_pile[p].draw();
	}
	
	for (p=0; p<PLAYERS; p++) {
		play_pile[p].unlock();
	}
}

#ifdef WIN32
void threaded_referee(void *argument_list)
#else
void *threaded_referee(void *argument_list)
#endif
{
	int three = 1;
	struct referee_arg_list_struct *p = (struct referee_arg_list_struct *) argument_list;

	//sched_setaffinity(0, 4, (cpu_set_t *)&three); 

	// wait for players to be ready  todo: make this work with signals
	while (PLAYERS != count_ready_players(p->player_ready)) {
		relinquish_exec;
	}

	// set the start bit to signal all players that the game can begin
	*(p->game_start_flag) = true;

	while (!*p->game_over) {
		if (PLAYERS == count_players(stuck, p->player_state_array)) {
			if (p->replenish_pile[0].size() > 0) { 
				turn_cards(p->play_pile, p->replenish_pile);
			} else {
				// there are no more cards to play
				*p->game_over = true;
			}
		}
		if (0 != count_players(done, p->player_state_array)) { // if (PLAYERS == counte_players(done,...
			*p->game_over = true;
		} else {
			relinquish_exec;
		}
	}

	// wait for all players to acknowledge end-of-game

	while (PLAYERS != count_players(done, p->player_state_array)) {
		relinquish_exec;
	}

#ifdef WIN32
	return;     
#else
	return 0;
#endif
}

enum deal_status deal(
	deck *playing_cards, 
	stack_of_cards *player_hand, 
	stack_of_cards *play_pile, 
	stack_of_cards *draw_pile, 
	stack_of_cards *replenish_pile
	)
{
	int c;
	int p;
	int i;

	playing_cards->shuffle();

	// deal hands

	c = 0;
	for (p=0; p<PLAYERS; p++) {
		for (i=0; i<CARDS_IN_HAND; i++) {
			player_hand[p] += playing_cards->card[c++];
		}

		play_pile[p] += playing_cards->card[c++];

		for (i=0; i<CARDS_IN_DRAW; i++) {
			draw_pile[p] += playing_cards->card[c++];
		}

		for (i=0; i<CARDS_IN_REPLENISH; i++) {
			replenish_pile[p] += playing_cards->card[c++];
		}
	}

	if (c != CARDS) {
		printf("The deal went bad, wrong number of cards! %d, expected %d \n", c, DECKS);
		return bad_deal;
	}

	return good_deal;
}

int play_speed (void)
{
	int p;
	bool game_over;
	deck playing_cards;
	stack_of_cards player_hand[PLAYERS];
	stack_of_cards play_pile[PLAYERS];
	stack_of_cards draw_pile[PLAYERS];
	stack_of_cards replenish_pile[PLAYERS];
	enum deal_status this_deal;

	this_deal = deal(&playing_cards, player_hand, play_pile, draw_pile, replenish_pile);

	if (this_deal == bad_deal) {
		return -1;
	}

	// play the game

	game_over = false;

#ifdef WIN32
	HANDLE player_handle[PLAYERS];
	HANDLE referee_handle;
#else
	pthread_t player_handle[PLAYERS];
	pthread_t referee_handle;
	int launch_status;
#endif
	struct player_arg_list_struct player_argument_list[PLAYERS];
	struct referee_arg_list_struct referee_argument_list;
	bool   game_start_flag = false;
	enum   player_state player_state_array[PLAYERS];
	bool   player_ready_bit[PLAYERS];

	for (p=0; p<PLAYERS; p++) {
		// create a thread for each player
					
		player_ready_bit[p] = false;
		player_state_array[p] = unlaunched;
		player_argument_list[p].my_hand      = &player_hand[p];
		player_argument_list[p].my_draw_pile = &draw_pile[p];
		player_argument_list[p].play_pile    = play_pile;
		player_argument_list[p].my_state     = &player_state_array[p];
		player_argument_list[p].player_ready = &player_ready_bit[p];
		player_argument_list[p].start_game   = &game_start_flag;
		player_argument_list[p].game_over    = &game_over;
		player_argument_list[p].player_no    = p;  // player #

#ifdef WIN32
		player_handle[p] = (HANDLE) _beginthread(threaded_player, 0, (void *) &(player_argument_list[p]));
#else
		launch_status = pthread_create(&player_handle[p], NULL, threaded_player, (void *) &player_argument_list[p]);
		printf("created player thread %u 0x%08X\n",p,(unsigned int)player_handle[p]);
		if (launch_status != 0) {
			printf("Failure to launch player%d: %08x \n",p,launch_status);
			perror("speed");
			exit(1);
		}			
#endif

	}
			
	referee_argument_list.player_state_array = player_state_array;
	referee_argument_list.replenish_pile = replenish_pile;
	referee_argument_list.player_hand = player_hand;
	referee_argument_list.play_pile = play_pile;
	referee_argument_list.player_ready = player_ready_bit;
	referee_argument_list.game_over = &game_over;
	referee_argument_list.game_start_flag = &game_start_flag;
#ifdef WIN32
	referee_handle = (HANDLE) _beginthread(threaded_referee, 0, &referee_argument_list);
#else
	launch_status = pthread_create(&referee_handle, NULL, threaded_referee, (void *) &referee_argument_list);
	if (launch_status != 0) {
		printf("Failure to launch referee: %08x\n", launch_status);
		perror("speed");
		exit(1);
	}			
#endif
	printf("created referee thread 0x%08X\n",(unsigned int)referee_handle);
/*
	SetThreadAffinityMask(player1_handle, 3);
	SetThreadAffinityMask(player2_handle, 3);
	SetThreadAffinityMask(referee_handle, 3);
*/
	// wait for the game to complete

#ifdef WIN32
	WaitForSingleObject(referee_handle, INFINITE);  
	for (p=0; p<PLAYERS; p++) {
		WaitForSingleObject(player_handle[p], INFINITE);  
	}
#else
	pthread_join( referee_handle, NULL);
	for (p=0; p<PLAYERS; p++) {
		pthread_join( player_handle[p], NULL);
	}
#endif
		
	if (!check_deck(0, &player_hand, &play_pile, &draw_pile, &replenish_pile, 0)) {
		return(6);
	}
/*
	if ((player_hand[0].size() == 0) && (player_hand[1].size() == 0)) {
//      printf("technically, player 1 wins - but 1 and 2 finished in the same round \n");
		return 0;
	}

	if (player_hand[0].size() == 0) {
//		printf("Player 1 wins! \n");
		return(1);
	} 

	if (player_hand[1].size() == 0) {
//		printf("Player 2 wins! \n");
		return(2);
	}

	if ((player_hand[0].size() + draw_pile[0].size()) == (player_hand[1].size() + draw_pile[1].size())) {
//		printf("A draw! \n");
		return(3);
	}

	if ((player_hand[0].size() + draw_pile[0].size()) < (player_hand[1].size() + draw_pile[1].size())) {
//		printf("Player 1 wins with fewer remaining cards \n");
		return(4);
	} else {
//		printf("Player 2 wins with fewer remaining cards \n");
		return(5);
	}
*/
	return 0;
}


int main(int argc, char** argv)
{
	int i;
	int j;
	FILE *f;
	time_t timer;
	unsigned long t;
        int r;

    if (argc < 2)
    {
    	printf("usage: parallel_speed <number games>");
    	return 0;
	}
    int games = atoi(argv[1]);

  //	f = fopen("stats.txt", "w");

	t = (unsigned long) time(&timer);

	for (i=0; i<games; i++) {
        printf("====> new game %d \n", i);
		r=play_speed();
	}
	t = (unsigned long) time(&timer) - t;
	printf("Elapsed time: %d \n", t);

	return 0;

	for (j=0; j<1; j++) {
		int stats[7] = { 0,0,0,0,0,0,0 };
		for (i=0; i<100; i++) {
			printf("i = %d \n", i);
			stats[play_speed()]++;
		}
/*
	 	printf(" a tie on speed %d \n", stats[0]);
		printf(" Player 1 won %5d \n", stats[1]); 
		printf(" Player 2 won %5d \n", stats[2]); 
		printf(" Draw %5d \n", stats[3]); 
		printf(" Player 1 won on cards %5d \n", stats[4]); 
		printf(" Player 2 won on cards %5d \n", stats[5]); 
*/
//		fprintf(f, "%d, %d, %d, %d, %d, %d \n", stats[0], stats[1], stats[2], stats[3], stats[4], stats[5], stats[6]);
	}

	t = (unsigned long) time(&timer) - t;
	printf("Elapsed time: %d \n", t);
//	fclose(f);
}
