lab2: game.c 
	gcc game.c dungeon.o -o game -lrt	# Compile and execute the game
	gcc barbarian.c -o barbarian -lrt	# Compile and execute barbarian next
	gcc wizard.c -o wizard -lrt	# Compile and execute wizard next
	gcc rogue.c -o rogue -lrt	# Lastly compile and execute rogue