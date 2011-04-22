CC = g++

OBJS   = stack_of_cards.o main.o random.o check_deck.o deck.o card.o 
HEADERS = card.h check_deck.h deck.h random.h stack_of_cards.h
TARGETS = speed

all:$(TARGETS)

speed : $(OBJS) $(HEADERS)
	$(CC) -g -o speed -lpthread $(OBJS)

%.o: %.cpp $(HEADERS)
	$(CC) $< -g -c $(CCFLAGS) 

clean:
	rm -rf $(TARGETS) $(OBJS)
