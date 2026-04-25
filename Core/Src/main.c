/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Escape the Island — Text RPG
  ******************************************************************************
  * @attention
  *
  * This game presents the user with a math question. It is played over the serial interface.
  * The user can reset the game with the on-board reset button of the NUCLEO board.
  * Baud Rate = 115200
  *
  * Copyright (c) 2023 STMicroelectronics.
  * Copyright (c) 2023 Dr. Billy Kihei, for CPE 2200
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum { //keeps track of state
    STATE_MAIN_MENU = 0,
    STATE_CHAR_SELECT,
    STATE_DIFF_SELECT,
    STATE_EXPLORE,
    STATE_DIALOGUE,
    STATE_INVENTORY,
    STATE_PUZZLE,
	STATE_TRAVEL,
	STATE_JUNGLE,
	STATE_CAVE,
	STATE_COASTLINE,
    STATE_NIGHT_EVENT,
	STATE_TOWER,
	STATE_BUNKER,
    STATE_WIN,
    STATE_GAME_OVER
} GameStateEnum;

typedef struct { //keeps track of status variables
    int8_t   health;        // 0-100, death at 0
    int8_t   fear;          // 0-100, collapse at 100
    int8_t   morale;        // 0-100, abandon at 0
    int8_t   starvation;    // 0-100, death at 100
    uint8_t  character;     // 0=Engineer 1=Medic 2=Survivalist 3=Skeptic
    uint8_t  difficulty;    // 0=Easy 1=Hard
    uint8_t  location;      // 0=Beach 1=Jungle 2=Cave 3=Coastline 4=Tower 5=Bunker
    uint8_t  day;
    uint8_t searches_done;
    uint8_t  inventory[8];  // 0=battery 1=wire 2=tools 3=map 4=rope 5=wood 6=knife 7=key
    uint16_t flags;
    uint8_t survivors[4]; // 1=present, 0=missing. 0=Engineer,1=Medic,2=Survivalist,3=Skeptic
    uint8_t notes[4];
    uint8_t  puzzles_solved;
    uint8_t puzzle_step;        // tracks progress through puzzle
    char    puzzle_input[8];    // stores player input
    uint8_t active_puzzle;      // 0=radio, 1=cave, 2=bunker
    GameStateEnum state;
    GameStateEnum prev_state;
} GameState;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define FLAG_MET_ENGINEER    (1 << 0)
#define FLAG_MET_MEDIC       (1 << 1)
#define FLAG_MET_SURVIVALIST (1 << 2)
#define FLAG_MET_SKEPTIC     (1 << 3)
#define FLAG_FOUND_KEY       (1 << 4)
#define FLAG_FIRE_LIT        (1 << 5)
#define FLAG_RAFT_BUILT      (1 << 6)
#define FLAG_HIGH_FEAR       (1 << 7)
#define FLAG_ENGINEER_HINT  (1 << 8)

#define ITEM_BATTERY  0
#define ITEM_WIRE     1
#define ITEM_TOOLS    2
#define ITEM_MAP      3
#define ITEM_ROPE     4
#define ITEM_WOOD     5
#define ITEM_KNIFE    6
#define ITEM_KEY      7

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */
volatile uint8_t dataAvail = 0;
char rx_buff[1];
GameState gs;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_DMA_Init(void);
/* USER CODE BEGIN PFP */
void UART_print(const char *str);
void game_init(void);
void show_main_menu(void);
void show_char_select(void);
void show_diff_select(void);
void show_explore(void);
void show_inventory(void);
void search_wreckage(void);
void show_survivor_list(void);
void show_dialogue(uint8_t survivor);
void show_map(void);
void show_travel_menu(void);
void show_jungle(void);
void search_jungle(void);
void show_current_location(void);
void show_cave(void);
void search_cave(void);
void cave_deeper(void);
void show_coastline(void);
void search_shipwreck(void);
void build_signal_fire(void);
void check_death_conditions(void);
void night_event(void);
void show_notes(void);
void show_tower(void);
void show_bunker(void);
void puzzle_radio(void);
void puzzle_cave(void);
void puzzle_bunker(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void UART_print(const char *str){
//define print function to make printing easier
    HAL_UART_Transmit(&huart2, (uint8_t *)str, strlen(str), 10);
}

void game_init(void){
    gs.health         = 100;
    gs.fear           = 0;
    gs.morale         = 75;
    gs.starvation     = 0;
    gs.character      = 0;
    gs.difficulty     = 0;
    gs.location       = 0;
    gs.day            = 1;
    memset(gs.survivors, 1, sizeof(gs.survivors));
    memset(gs.notes, 0, sizeof(gs.notes));
    gs.flags          = 0;
    gs.puzzles_solved = 0;
    gs.puzzle_step  = 0;
    gs.active_puzzle = 0;
    memset(gs.puzzle_input, 0, sizeof(gs.puzzle_input));
    gs.searches_done  = 0;
    memset(gs.inventory, 0, sizeof(gs.inventory));
    gs.state          = STATE_MAIN_MENU;
    gs.prev_state     = STATE_MAIN_MENU;
}

void show_main_menu(void){
    UART_print("\r\n");
    UART_print("##################################################\r\n");
    UART_print("#                                                #\r\n");
    UART_print("#           ESCAPE THE ISLAND                    #\r\n");
    UART_print("#                                                #\r\n");
    UART_print("#                                                #\r\n");
    UART_print("##################################################\r\n");
    UART_print("\r\n");
    UART_print("  [P] Play\r\n");
    UART_print("  [X] Exit\r\n");
    UART_print("\r\n> ");
}

void show_char_select(void){
    UART_print("\r\n-- Choose your survivor --\r\n\r\n");
    UART_print("  [1] Engineer    - better puzzle outcomes\r\n");
    UART_print("  [2] Medic       - starts with higher health\r\n");
    UART_print("  [3] Survivalist - better resource finds\r\n");
    UART_print("  [4] Skeptic     - reduced morale for others\r\n");
    UART_print("\r\n> ");
}

void show_diff_select(void){
    UART_print("\r\n-- Choose difficulty --\r\n\r\n");
    UART_print("  [E] Easy - hints enabled, more resources\r\n");
    UART_print("  [H] Hard - no hints, scarce resources\r\n");
    UART_print("\r\n> ");
}

void show_explore(void){
    UART_print("\r\n== BEACH ==\r\n");
    if (gs.fear < 34) {
        UART_print("Warm sand stretches around you. Survivors tend a small fire.\r\n");
    } else if (gs.fear < 67) {
        UART_print("The beach feels exposed. Every sound makes you flinch.\r\n");
    } else {
        UART_print("The waves sound wrong. The other survivors won't meet your eyes.\r\n");
    }
    UART_print("\r\n  [1] Search wreckage\r\n");
    UART_print("  [2] Talk to survivors\r\n");
    UART_print("  [3] View map\r\n");
    UART_print("  [4] View inventory\r\n");
    UART_print("  [5] Rest (advance day)\r\n");
    UART_print("  [6] Travel to another location\r\n");
    UART_print("  [7] Read notes\r\n");
    UART_print("\r\n> ");
}
void show_inventory(void){
	const char *item_names[8] ={
			"Battery", "Wire", "Tool Kit", "Map",
	"Rope", "Wood", "Knife", "Old Key"
	};
	UART_print("\r\n--Inventory--\r\n");
	uint8_t empty = 1;
	for (int i=0; i<8; i++){
		if (gs.inventory[i]){
			UART_print("[X]");
			UART_print(item_names[i]);
			UART_print("\r\n");
			empty=0;
		}
	}
	if (empty) UART_print("Nothing in Inventory yet.\r\n");
	UART_print("\r\nPress any key to continue...\r\n");
}
void search_wreckage(void){
	uint8_t roll = (gs.day*37+gs.health+13)%3; //pseudo-random number generator (modified LCG)
	if (gs.searches_done >= 3) {
	    UART_print("\r\nThe wreckage has been picked clean. Nothing left.\r\n");
	}
	else if (gs.inventory[roll]){
		UART_print("\r\nYou dig through the wreckage but find nothing new.\r\n");
		gs.searches_done++;
	}
	else {
		const char *item_names[3] ={"Battery", "Wire", "Tool Kit"};
		gs.inventory[roll] =1;
		UART_print("\r\nYou found: ");
		UART_print(item_names[roll]);
		UART_print("!\r\n");
		gs.searches_done++;
	}
	gs.day++; //Searching takes up entire day
	UART_print("\r\nPress any key to continue...\r\n");
}
void show_survivor_list(void){
	UART_print("\r\n-- Survivors --\r\n\r\n");
	if (gs.survivors[0]) UART_print("  [1] Engineer\r\n");
	if (gs.survivors[1]) UART_print("  [2] Medic\r\n");
	if (gs.survivors[2]) UART_print("  [3] Survivalist\r\n");
	if (gs.survivors[3]) UART_print("  [4] Skeptic\r\n");
	UART_print("\r\n> ");
}
void show_dialogue(uint8_t survivor){
	if (!gs.survivors[survivor]) {
	UART_print("\r\nThat survivor is no longer here.\r\n");
	UART_print("\r\nPress any key to continue...\r\n");
	return;
	}
	switch(survivor){
		case 0: //Engineer
			gs.flags |=FLAG_MET_ENGINEER; //bitwise OR. met engineer flag bit=1
			if (gs.character == 0) {
			    UART_print("\r\nEngineer: \"Another engineer? Good. Don't get in my way.\"\r\n");
			}
			else if (gs.flags & FLAG_FOUND_KEY){
				UART_print(("\r\nEngineer: \"That key you found... I think it opens something near the bunker.\"\r\n"));
			}
			else if (gs.puzzles_solved & 0x01) {
				UART_print("\r\nEngineer: \"Good work on the radio. We might actually get out of here.\"\r\n");
			}
			else if ((gs.difficulty == 1) && FLAG_FOUND_KEY) {
				gs.flags |= FLAG_ENGINEER_HINT;
				UART_print("\r\nEngineer: \"If we can find a battery and some wire, I can fix the radio tower.\"\r\n");
				UART_print("Engineer: \"The panel has four wires. Blue carries the signal.\"\r\n");
				UART_print("Engineer: \"Red grounds the charge. Yellow is the warning wire.\"\r\n");
				UART_print("Engineer: \"Green completes the loop. Get the order right or it'll short out.\"\r\n");
			}
			else {
				UART_print("\r\nEngineer: \"If we can find a battery and some wire, I can fix the radio tower.\"\r\n");
			}
			break;
		case 1: //Medic
			gs.flags |= FLAG_MET_MEDIC;
			if (gs.character == 1) {
			     UART_print("\r\nMedic: \"I don't have enough supplies for both of us. Please be careful.\"\r\n");
			}
			else {
				gs.health += 10; //medic undoes damage
				if (gs.health >100) gs.health = 100; //limit health to 100
				UART_print("\r\nMedic: \"Hold still... I'll patch you up.\"\r\n");
				UART_print("Your health increased.\r\n");
			}
			break;
		case 2: //Survivalist
			gs.flags |= FLAG_MET_SURVIVALIST;
			if (gs.character == 2){
				UART_print("\r\nSurvivalist: \"You know as well as I do.. jungle has rope and wood. Stop wasting time.\"\r\n");
			}
			else if (gs.flags & FLAG_RAFT_BUILT){
				UART_print("\r\nSurvivalist: \"The raft is ready. Say the word and we leave.\"\r\n");
			}
			else {
				UART_print("\r\nSurvivalist: \"We need rope and wood to build a raft. I've seen both in the jungle.\"\r\n");
			}
			break;
		 case 3: // Skeptic
		    gs.flags |= FLAG_MET_SKEPTIC;
		    if (gs.character == 3) {
		        UART_print("\r\nSkeptic: \"At least you're honest about our situation. Everyone else is delusional.\"\r\n");
		    }
		    else {
		        gs.morale -= 10;
		        if (gs.morale < 0) gs.morale = 0;
		        UART_print("\r\nSkeptic: \"Nobody is coming for us. We're going to die here.\"\r\n");
		        UART_print("Morale decreased.\r\n");
		        }
		    break;
	}
}
void show_map(void)
{
    UART_print("\r\n");
    UART_print("  ==========================================\r\n");
    UART_print("              I S L A N D   M A P\r\n");
    UART_print("  ==========================================\r\n");

    UART_print("\r\n    ");
    UART_print(gs.location == 1 ? "[JUNGLE*]" : "[JUNGLE] ");
    UART_print("----------");
    UART_print(gs.location == 4 ? "[TOWER*] " : "[TOWER]  ");
    UART_print("\r\n");

    UART_print("        |                    |\r\n");
    UART_print("        |      (ISLAND)      |\r\n");
    UART_print("        |                    |\r\n");

    UART_print("    ");
    UART_print(gs.location == 0 ? "[BEACH*] " : "[BEACH]  ");
    UART_print("----");
    UART_print(gs.location == 3 ? "[COAST*] " : "[COAST]  ");
    UART_print("----");
    UART_print(gs.location == 2 ? "[CAVE*]  " : "[CAVE]   ");
    UART_print("\r\n");

    UART_print("                             |\r\n");
    UART_print("                         ");
    UART_print(gs.location == 5 ? "[BUNKER*]" : "[BUNKER] ");
    UART_print("\r\n");

    UART_print("\r\n  ==========================================\r\n");

    const char *loc_names[6] = {
        "BEACH", "JUNGLE", "CAVE", "COASTLINE", "RADIO TOWER", "BUNKER"
    };
    UART_print("  Location : ");
    UART_print(loc_names[gs.location]);
    UART_print("\r\n  Day      : ");
    char daybuf[3] = {'0' + gs.day, '\0'};
    UART_print(daybuf);
    UART_print("\r\n  * = you are here\r\n");
    UART_print("  ==========================================\r\n");
    UART_print("\r\nPress any key to continue...\r\n");
}
void show_travel_menu(void){
	//if current location -> print (you are here)
	UART_print("\r\n-- Where do you want to go? --\r\n\r\n");
	UART_print(gs.location == 0 ? "  [1] Beach (you are here)\r\n" : "  [1] Beach\r\n");
	UART_print(gs.location == 1 ? "  [2] Jungle (you are here)\r\n" : "  [2] Jungle\r\n");
	UART_print(gs.location == 2 ? "  [3] Cave (you are here)\r\n" : "  [3] Cave\r\n");
	UART_print(gs.location == 3 ? "  [4] Coastline (you are here)\r\n" : "  [4] Coastline\r\n");
	if (gs.inventory[ITEM_MAP])
	        UART_print(gs.location == 4 ? "  [5] Radio Tower (you are here)\r\n" : "  [5] Radio Tower\r\n");
	if (gs.inventory[ITEM_KEY])
	        UART_print(gs.location == 5 ? "  [6] Bunker (you are here)\r\n" : "  [6] Bunker\r\n");
	UART_print("\r\n> ");
}
void show_jungle(void){
    UART_print("\r\n== JUNGLE ==\r\n");
    if (gs.fear < 34) {
        UART_print("Thick canopy filters the sunlight. Birds call in the distance.\r\n");
    }
    else if (gs.fear < 67) {
        UART_print("The jungle feels like it's watching you. Every rustle makes you tense.\r\n");
    }
    else {
        UART_print("The trees press in around you. Something is out there. You're sure of it.\r\n");
    }
    UART_print("\r\n  [1] Search for materials\r\n");
    UART_print("  [2] Hunt for food\r\n");
    UART_print("  [3] View map\r\n");
    UART_print("  [4] View inventory\r\n");
    UART_print("  [5] Travel\r\n");
    UART_print("  [6] Rest (advance day)\r\n");
    UART_print("  [7] Read notes\r\n");
    UART_print("\r\n> ");
}
void search_jungle(void){
	uint8_t roll = (gs.day*17+7)%2; //gives 0 or 1
	uint8_t item = roll+4; //Jungle items: items 4 & 5
	if (gs.inventory[item]){
		UART_print("\r\nYou search but find nothing new.\r\n");
	}
	else{
		const char *item_names[2] = {"Rope", "Wood"};
		        gs.inventory[item] = 1; //set item to found
		        UART_print("\r\nYou find: ");
		        UART_print(item_names[roll]);
		        UART_print("!\r\n");
	}
	gs.day++;
	UART_print("\r\nPress any key to continue...\r\n");
}
void hunt_jungle(void){
	if(gs.starvation > 20){
		gs.starvation -=20;
		UART_print("\r\nYou catch a small animal.\r\n");
		UART_print("Starvation decreased.\r\n");
	}
	else {
		UART_print("\r\nYou find nothing. Better luck next time.");
	}
	gs.day++;
	UART_print("\r\nPress any key to continue...\r\n");
}
void show_current_location(void){
    switch(gs.location){
        case 0: show_explore();    break;
        case 1: show_jungle();     break;
        case 2: show_cave();    break;
        case 3: show_coastline();    break;
        case 4: show_tower();   break;
        case 5: show_bunker();  break;
    }
}
void show_cave(void){
	UART_print("\r\n== CAVE ==\r\n");
	if (gs.fear < 34) {
	    UART_print("A cave entrance waits ahead. Cool air drifts from within.\r\n");
	}
	else if (gs.fear < 67) {
	    UART_print("The cave is darker than you remembered. Your torch flickers.\r\n");
	}
	else {
	    UART_print("Something breathes in the dark. You tell yourself it's the wind.\r\n");
	}
	UART_print("\r\n  [1] Search the cave\r\n");
	UART_print("  [2] Go deeper\r\n");
	UART_print("  [3] View map\r\n");
	UART_print("  [4] View inventory\r\n");
	UART_print("  [5] Travel\r\n");
	UART_print("  [6] Rest (advance day)\r\n");
	UART_print("  [7] Read notes\r\n");
	UART_print("\r\n> ");
}
void search_cave(void){
	if (gs.inventory[ITEM_KEY]){
		UART_print("\r\nYou've already found everything useful here.\r\n");
	}
	else {
		gs.inventory[ITEM_KEY]=1;
		gs.flags |= FLAG_FOUND_KEY;
		UART_print("\r\nHidden beneath the rocks lies a rusty old key... as if waiting to be found.\r\n");
		UART_print("\r\n You pocket the key, a faint chill following your touch\r\n");
	}
	gs.day++;
	UART_print("\r\nPress any key to continue...\r\n");
}
void cave_deeper(void){
	gs.fear += 10;
	if (gs.fear > 100) gs.fear = 100;
	UART_print("\r\nYou descend deeper into the cave.\r\n");
	UART_print("The walls tighten around you. The air thins.\r\n");
	UART_print("A chamber waits ahead, its only passage buried in rubble.\r\n");
	puzzle_cave();
}
void show_coastline(void){
    UART_print("\r\n== COASTLINE ==\r\n");
    if (gs.fear < 34) {
        UART_print("Waves crash against jagged rocks. You spot a shipwreck off in the distance.\r\n");
    }
    else if (gs.fear < 67) {
        UART_print("The tide is coming in fast. The shipwreck groans in the current.\r\n");
    }
    else{
        UART_print("The ocean stretches endlessly. No ships. No planes. Nothing is coming.\r\n");
    }
    UART_print("\r\n  [1] Search the shipwreck\r\n");
    UART_print("  [2] Build signal fire\r\n");
    UART_print("  [3] View map\r\n");
    UART_print("  [4] View inventory\r\n");
    UART_print("  [5] Travel\r\n");
    UART_print("  [6] Rest (advance day)\r\n");
    UART_print("  [7] Read notes\r\n");
    UART_print("\r\n> ");
}
void search_shipwreck(void){
    uint8_t roll = (gs.day * 23 + gs.fear + 11) % 2; // 0 or 1
    if (gs.inventory[6] && gs.inventory[ITEM_MAP]) {
        UART_print("\r\nThe shipwreck has nothing left to offer.\r\n");
    }
    else {
        if (roll == 0 && !gs.inventory[ITEM_KNIFE]) {
            gs.inventory[ITEM_KNIFE] = 1;
            UART_print("\r\nYou find a Knife wedged in the hull.\r\n");
        }
        else if (roll == 1 && !gs.inventory[ITEM_MAP]) {
            gs.inventory[ITEM_MAP] = 1;
            UART_print("\r\nYou find a waterlogged Map. Still readable.\r\n");
        }
        else {
            UART_print("\r\nYou search but find nothing new this time.\r\n");
        }
    }
    gs.day++;
    UART_print("\r\nPress any key to continue...\r\n");
}
void build_signal_fire(void){
    if (!(gs.flags & FLAG_FIRE_LIT)) {
        gs.flags |= FLAG_FIRE_LIT;
        gs.morale += 15;
        if (gs.morale > 100) gs.morale = 100;
        UART_print("\r\nYou gather driftwood and light a signal fire.\r\n");
        UART_print("Smoke rises into the sky. Someone might see it.\r\n");
        UART_print("Morale increased.\r\n");
    }
    else {
        UART_print("\r\nThe signal fire is already burning.\r\n");
        UART_print("All you can do now is wait and hope.\r\n");
    }
    gs.day++;
    UART_print("\r\nPress any key to continue...\r\n");
}
void check_death_conditions(void){
    if (gs.health <= 0) {
        gs.state = STATE_GAME_OVER;
        UART_print("\r\n##################################################\r\n");
        UART_print("#                                                #\r\n");
        UART_print("#                 YOU DIED                       #\r\n");
        UART_print("#         Your injuries were too severe.         #\r\n");
        UART_print("#                                                #\r\n");
        UART_print("##################################################\r\n");
        UART_print("\r\nPress reset to play again.\r\n");
        while(1);
    }
    if (gs.fear >= 100) {
        gs.state = STATE_GAME_OVER;
        UART_print("\r\n##################################################\r\n");
        UART_print("#                                                #\r\n");
        UART_print("#            PSYCHOLOGICAL COLLAPSE              #\r\n");
        UART_print("#     The island broke you. You gave up.         #\r\n");
        UART_print("#                                                #\r\n");
        UART_print("##################################################\r\n");
        UART_print("\r\nPress reset to play again.\r\n");
        while(1);
    }
    if (gs.morale <= 0) {
        gs.state = STATE_GAME_OVER;
        UART_print("\r\n##################################################\r\n");
        UART_print("#                                                #\r\n");
        UART_print("#               ABANDONED                        #\r\n");
        UART_print("#   The group fell apart. You were left behind.  #\r\n");
        UART_print("#                                                #\r\n");
        UART_print("##################################################\r\n");
        UART_print("\r\nPress reset to play again.\r\n");
        while(1);
    }
}
void night_event(void){
    uint8_t roll = (gs.day * 31 + gs.fear * 7 + gs.health * 3 + gs.morale) % 6;
    UART_print("\r\n-- Night falls --\r\n\r\n");
    switch(roll)
    {
        case 0: // Storm
            gs.fear += 15;
            if (gs.fear > 100) gs.fear = 100;
            UART_print("A violent storm rolls in. Lightning splits the sky.\r\n");
            UART_print("Fear increased.\r\n");
            break;
        case 1: // Animal attack
            gs.health -= 15;
            gs.fear += 10;
            if (gs.fear > 100) gs.fear = 100;
            UART_print("Something attacks your camp in the night.\r\n");
            UART_print("You fight it off but take injuries.\r\n");
            UART_print("Health decreased. Fear increased.\r\n");
            break;
        case 2: // Footsteps
            gs.fear += 20;
            if (gs.fear > 100) gs.fear = 100;
            UART_print("You hear footsteps circling the camp.\r\n");
            UART_print("By morning, whoever it was is gone.\r\n");
            UART_print("Fear increased.\r\n");
            break;
        case 3: // Survivor missing
        {
            uint8_t victim = (gs.day * 13 + gs.fear) % 4;
            gs.survivors[victim] = 0;
            gs.notes[victim] = 1;
            gs.morale -= 20;
            if (gs.morale < 0) gs.morale = 0;
            UART_print("You wake to find one of the survivors gone.\r\n");
            // each victim leaves a note
            switch(victim)
            {
                case 0:
                    UART_print("You find a scrawled note:\r\n");
                    UART_print("'Battery + wire + tools = radio tower. --Engineer'\r\n");
                    break;
                case 1:
                    UART_print("You find a medical kit left behind with a note:\r\n");
                    UART_print("'Use this. Stay alive. --Medic'\r\n");
                    gs.health += 10;
                    if (gs.health > 100) gs.health = 100;
                    break;
                case 2:
                    UART_print("You find a note scratched into the sand:\r\n");
                    UART_print("'Rope + wood + knife = raft. Trust no one. --Survivalist'\r\n");
                    break;
                case 3:
                    UART_print("You find a note:\r\n");
                    UART_print("'We were never getting off this island. --Skeptic'\r\n");
                    gs.morale -= 10;
                    if (gs.morale < 0) gs.morale = 0;
                    break;
            }
            UART_print("Morale decreased.\r\n");
            break;
        }
        case 4: // Fire goes out
            if (gs.flags & FLAG_FIRE_LIT) {
                gs.flags &= ~FLAG_FIRE_LIT;
                gs.morale -= 10;
                if (gs.morale < 0) gs.morale = 0;
                UART_print("The signal fire went out in the night.\r\n");
                UART_print("Morale decreased.\r\n");
            }
            else {
                UART_print("A quiet night. You sleep better than expected.\r\n");
                gs.morale += 5;
                if (gs.morale > 100) gs.morale = 100;
            }
            break;
        case 5: // Peaceful night
            gs.morale += 10;
            gs.health += 5;
            if (gs.morale > 100) gs.morale = 100;
            if (gs.health > 100) gs.health = 100;
            UART_print("A rare peaceful night. The stars are beautiful.\r\n");
            UART_print("Health and morale increased.\r\n");
            break;
    }
    if (gs.fear >= 70) gs.flags |= FLAG_HIGH_FEAR;
    UART_print("\r\nPress any key to continue...\r\n");
    check_death_conditions();
}
void show_notes(void){
    UART_print("\r\n-- Notes --\r\n\r\n");
    uint8_t any = 0;
    if (gs.notes[0]) {
        UART_print("Engineer: 'Battery + wire + tools = radio tower.'\r\n");
        any = 1;
        if (gs.difficulty==1){
        	gs.flags |= FLAG_ENGINEER_HINT;
        	UART_print("When you get in: The panel has four wires. Blue carries the signal.\r\n");
        	UART_print("Red grounds the charge. Yellow is the warning wire.\r\n");
        	UART_print("Green completes the loop. Get the order right or it'll short out.\r\n");
        }
    }
    if (gs.notes[1]) {
        UART_print("Medic: 'Stay alive.'\r\n");
        any = 1;
    }
    if (gs.notes[2]) {
        UART_print("Survivalist: 'Rope + wood + knife = raft.'\r\n");
        any = 1;
    }
    if (gs.notes[3]) {
        UART_print("Skeptic: 'We were never getting off this island.'\r\n");
        any = 1;
    }
    if (!any) UART_print("No notes yet.\r\n");
    UART_print("\r\nPress any key to continue...\r\n");
}
void show_tower(void){
    UART_print("\r\n== RADIO TOWER ==\r\n");
    if (gs.fear < 34) {
        UART_print("A rusted radio tower rises above the trees. It might still work.\r\n");
    }
    else if (gs.fear < 67) {
        UART_print("The tower sways in the wind. Loose wires whip and dangle in every direction.\r\n");
    }
    else {
        UART_print("The tower looks like it could collapse any moment. Do you really want to climb it?\r\n");
    }
    UART_print("\r\n  [1] Attempt radio puzzle\r\n");
    UART_print("  [2] View map\r\n");
    UART_print("  [3] View inventory\r\n");
    UART_print("  [4] Read notes\r\n");
    UART_print("  [5] Travel\r\n");
    UART_print("\r\n> ");
}
void show_bunker(void){
    UART_print("\r\n== BUNKER ==\r\n");
    if (gs.fear < 34) {
        UART_print("A bunker door is built into the hillside. The key fits the lock.\r\n");
    } else if (gs.fear < 67) {
        UART_print("The bunker door is cold to the touch. Something hums inside.\r\n");
    } else {
        UART_print("You don't want to know what's in there, but what choice do you have?\r\n");
    }
    UART_print("\r\n  [1] Attempt generator puzzle\r\n");
    UART_print("  [2] View map\r\n");
    UART_print("  [3] View inventory\r\n");
    UART_print("  [4] Read notes\r\n");
    UART_print("  [5] Travel\r\n");
    UART_print("\r\n> ");
}
void puzzle_radio(void)
{
    // Check required items
    if (!gs.inventory[ITEM_BATTERY] || !gs.inventory[ITEM_WIRE] ||
        !gs.inventory[ITEM_TOOLS]   || !gs.inventory[ITEM_MAP]) {
        UART_print("\r\nYou don't have everything you need.\r\n");
        UART_print("Required: Battery, Wire, Tool Kit, Map\r\n");
        UART_print("\r\nPress any key to continue...\r\n");
        return;
    }

    UART_print("\r\n== RADIO TOWER PUZZLE ==\r\n");
    UART_print("The radio panel has four wire slots: R G B Y\r\n");
    UART_print("You need to connect them in the correct order.\r\n\r\n");
    if (gs.difficulty == 0) {
        UART_print("Four wires must be connected in order:\r\n\r\n");
        UART_print("  - Red is not last, but Blue comes before Red.\r\n");
        UART_print("  - Yellow comes after Red, but before Green.\r\n");
    }
    else {
        if(gs.character == 0){
    	    	UART_print("  - The wire that carries the signal starts the circuit.\r\n");
    	    	UART_print("  - The wire that grounds the charge follows immediately.\r\n");
    	    	UART_print("  - The warning wire comes third.\r\n");
    	    	UART_print("  - The wire that completes the loop is last.\r\n\r\n");
    	    	UART_print(" You recall that blue carries the signal, yellow is the warning wire, and red grounds the charge");
    	    	}
        else if (gs.flags & FLAG_ENGINEER_HINT) {
    	        UART_print("  - The wire that carries the signal starts the circuit.\r\n");
    	        UART_print("  - The wire that grounds the charge follows immediately.\r\n");
    	        UART_print("  - The warning wire comes third.\r\n");
    	        UART_print("  - The wire that completes the loop is last.\r\n\r\n");
    	        UART_print("Maybe the engineer's hint could be used to solve this..");
    	    }
    	else {
    	        UART_print("The panel is unlabeled. You'll need to figure this out yourself.\r\n");
    	        UART_print("Maybe someone on the island knows something...\r\n\r\n");
    	    }
    }
    UART_print("Enter the correct sequence (4 letters, e.g. BRGY):\r\n");
    UART_print("> ");
    // Switch to puzzle input mode
    gs.state = STATE_PUZZLE;
    gs.puzzle_step = 0;
    memset(gs.puzzle_input, 0, sizeof(gs.puzzle_input));
}
void puzzle_cave(void){
    if (!(gs.flags & FLAG_FOUND_KEY)) {
        UART_print("\r\nThe chamber is blocked. Search around some more, and try again later.\r\n");
        UART_print("\r\nPress any key to continue...\r\n");
        return;
    }
    UART_print("\r\n== CAVE PUZZLE ==\r\n");
    UART_print("You squeeze into a sealed chamber.\r\n");
    UART_print("The air is thin. Debris blocks the passage ahead.\r\n\r\n");
    UART_print("Three piles block the way: A, B, and C.\r\n\r\n");
    if (gs.difficulty == 0) {
    	UART_print("A quick inspection reveals:\r\n");
    	UART_print("  - Pile C is not structurally connected to the base layer, but it is locked by the stability of A.\r\n");
    	UART_print("  - Pile B is blocking access to Pile A.\r\n");
    	UART_print("  - Pile A must be cleared before anything behind it can shift.\r\n");
    }
    else {
        UART_print("The air grows thinner with every second you hesitate.\r\n");
        UART_print("The structural integrity is critical:\r\n");
        UART_print("  - Pile C is immovable while Pile A maintains its current tension.\r\n");
        UART_print("  - Pile A is structurally braced by the position of Pile B.\r\n");
        UART_print("  - Pile B is the only mass currently free of external load.\r\n\r\n");
    }
    UART_print("Which pile do you move first? (A/B/C):\r\n> ");
    gs.active_puzzle = 1;
    gs.puzzle_step = 0;
    gs.state = STATE_PUZZLE;
}
void puzzle_bunker(void){
    if (!gs.inventory[ITEM_BATTERY] || !gs.inventory[ITEM_WIRE]) {
        UART_print("\r\nThe generator needs power components.\r\n");
        UART_print("Required: Battery, Wire\r\n");
        UART_print("\r\nPress any key to continue...\r\n");
        return;
    }
    UART_print("\r\n== BUNKER PUZZLE ==\r\n");
    UART_print("A rusted generator sits in the corner.\r\n");
    UART_print("Three steps to get it running:\r\n\r\n");
    UART_print("  [1] Connect the battery\r\n");
    UART_print("  [2] Route the wiring\r\n");
    UART_print("  [3] Prime the fuel line\r\n\r\n");
    if (gs.difficulty == 0) {
        UART_print("Hint: Prime the fuel line before connecting anything.\r\n");
        UART_print("Connecting power before priming will short the system.\r\n\r\n");
    }
    else {
    	if (gs.character == 0){
    		UART_print("You recall you should always prime mechanical systems before applying power.\r\n\r\n");
    	}
        if (gs.flags & FLAG_ENGINEER_HINT) {
            UART_print("You recall the Engineer mentioning something about\r\n");
            UART_print("always priming mechanical systems before applying power.\r\n\r\n");
        }
        else {
            UART_print("The generator looks complex. Maybe someone on the\r\n");
            UART_print("island knows how these systems work...\r\n\r\n");
        }
    }
    UART_print("Choose your first step (1/2/3):\r\n> ");
    gs.active_puzzle = 2;
    gs.puzzle_step = 0;
    gs.state = STATE_PUZZLE;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_DMA_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart2, (uint8_t *)rx_buff, 1);
  game_init();
  show_main_menu();
  /* USER CODE END 2 */

  /* Game engine runs in infinite loop until finished. */
  /* Game engine BEGIN WHILE */
  while(1)
  {
      if (!dataAvail) continue;

      char c = rx_buff[0]; //take char from UART buffer
      dataAvail = 0; //clear flag

      switch (gs.state)
      {
          case STATE_MAIN_MENU:
              if (c == 'p' || c == 'P') {
                  gs.state = STATE_CHAR_SELECT;
                  show_char_select();
              } else if (c == 'x' || c == 'X') {
                  UART_print("\r\nGoodbye. Press reset to play again.\r\n");
                  while(1);
              }
              break;

          case STATE_CHAR_SELECT:
              if (c >= '1' && c <= '4') {
                  gs.character = c - '1';
                  gs.state = STATE_DIFF_SELECT;
                  show_diff_select();
              }
              break;

          case STATE_DIFF_SELECT:
              if (c == 'e' || c == 'E') {
                  gs.difficulty = 0;
                  gs.state = STATE_EXPLORE;
                  show_explore();
              } else if (c == 'h' || c == 'H') {
                  gs.difficulty = 1;
                  gs.state = STATE_EXPLORE;
                  show_explore();
              }
              break;

          case STATE_EXPLORE:
        	  if (c == '1'){
        		  gs.prev_state = STATE_EXPLORE;
        		  gs.state = STATE_INVENTORY; //use to return to prev. state
        		  search_wreckage();
        	  }
        	  else if (c == '2'){
        		  gs.state = STATE_DIALOGUE;
        		  show_survivor_list();
        	  }
        	  else if (c == '3'){
        		  gs.prev_state = STATE_EXPLORE;
        		  gs.state = STATE_INVENTORY;
        		  show_map();
        	  }
        	  else if (c == '4'){
        		  gs.prev_state = STATE_EXPLORE;
        		  gs.state = STATE_INVENTORY;
        		  show_inventory();

        	  }
        	  else if (c == '5') {
        		  gs.prev_state = gs.state;
        		  gs.state = STATE_INVENTORY;
        		  gs.day++;
        		  night_event();
              }
        	  else if (c == '6'){
        		  gs.state = STATE_TRAVEL;
        		  show_travel_menu();
        	  }
        	  else if (c == '7'){
        		  gs.prev_state = STATE_EXPLORE;
        		  gs.state = STATE_INVENTORY;
        		  show_notes();
        	  }
              break;
          case STATE_INVENTORY:
        	  //return to previous state
        	  gs.state = gs.prev_state;
        	  show_current_location();
        	  break;
          case STATE_DIALOGUE:
        	  if (c>='1' && c <= '4'){
        		  show_dialogue(c-'1');
        		  gs.state = STATE_INVENTORY;
        		  gs.prev_state = STATE_EXPLORE;
        	  }
        	  else if (c=='5'){ //Leave
        		  gs.state = STATE_EXPLORE;
        		  show_explore();
        	  }
        	  break;
          case STATE_TRAVEL:
        	  if (c=='1'){
        		  gs.location = 0; //beach
        		  gs.state = STATE_EXPLORE;
        		  show_explore();
        	  }
        	  else if (c == '2'){
        		  gs.location = 1; //Jungle
        		  gs.state = STATE_JUNGLE;
        		  show_jungle();
        	  }
        	  else if (c == '3'){
        		  gs.location = 2; //Cave
        		  gs.state = STATE_CAVE;
        		  show_cave();
        	  }
        	  else if (c == '4'){
        		  gs.location = 3; //Coastline
        		  gs.state = STATE_COASTLINE;
        		  show_coastline();
        	  }
        	  else if (c == '5' && gs.inventory[ITEM_MAP]) {
        	      gs.location = 4; //tower
        	      gs.state = STATE_TOWER;
        	      show_tower();
        	  }
        	  else if (c == '6' && gs.inventory[ITEM_KEY]) {
        	      gs.location = 5; //bunker
        	      gs.state = STATE_BUNKER;
        	      show_bunker();
        	  }
        	  break;
          case STATE_JUNGLE:
        	  if (c == '1'){ //search for materials
        		  gs.prev_state = STATE_JUNGLE;
        		  gs.state = STATE_INVENTORY;
        		  search_jungle();
        	  }
        	  else if (c == '2'){ //Hunt for food
        		  gs.prev_state = STATE_JUNGLE;
        		  gs.state = STATE_INVENTORY;
        		  hunt_jungle();
        	  }
        	  else if (c == '3'){ //view map
        		  gs.prev_state = STATE_JUNGLE;
        		  gs.state = STATE_INVENTORY;
        		  show_map();
        	  }
        	  else if (c == '4'){ //view inventory
        		  gs.prev_state = STATE_JUNGLE;
        		  gs.state = STATE_INVENTORY;
        		  show_inventory();
        	  }
        	  else if (c == '5'){//travel
        		  gs.state = STATE_TRAVEL;
        		  show_travel_menu();
        	  }
        	  else if (c == '6') {
        	      gs.prev_state = gs.state;
        	      gs.state = STATE_INVENTORY;
        	      gs.day++;
        	      night_event();
        	  }
        	  else if (c == '7') {
        	      gs.prev_state = gs.state;
        	      gs.state = STATE_INVENTORY;
        	      show_notes();
        	  }
        	  break;
          case STATE_CAVE:
        	  if (c == '1'){ //search  the cave
        		  gs.prev_state = STATE_CAVE;
        		  gs.state = STATE_INVENTORY;
        		  search_cave();
        	  }
        	  else if (c == '2'){ //go deeper
        		  gs.prev_state = STATE_CAVE;
        		  gs.state = STATE_INVENTORY;
        		  cave_deeper();
        	  }
        	  else if (c == '3'){ //view map
        		  gs.prev_state = STATE_CAVE;
        		  gs.state = STATE_INVENTORY;
        		  show_map();
        	  }
        	  else if (c == '4'){ //view inventory
        		  gs.prev_state = STATE_CAVE;
        		  gs.state = STATE_INVENTORY;
        		  show_inventory();
        	  }
        	  else if (c == '5'){ //travel
        		  gs.state = STATE_TRAVEL;
        		  show_travel_menu();
        	  }
        	  else if (c == '6') {
        	       gs.prev_state = gs.state;
        	       gs.state = STATE_INVENTORY;
        	       gs.day++;
        	       night_event();
        	  }
        	  else if (c == '7') {
        	       gs.prev_state = gs.state;
        	       gs.state = STATE_INVENTORY;
        	       show_notes();
        	  }
        	  break;
          case STATE_COASTLINE:
        	  if (c == '1'){ //search shipwreck
        		  gs.prev_state = STATE_COASTLINE;
        		  gs.state = STATE_INVENTORY;
        		  search_shipwreck();
        	  }
        	  else if (c == '2'){ //build signal fire
        		  gs.prev_state = STATE_COASTLINE;
        		  gs.state = STATE_INVENTORY;
        		  build_signal_fire();
        	  }
        	  else if (c == '3'){ //view map
        		  gs.prev_state = STATE_COASTLINE;
        		  gs.state = STATE_INVENTORY;
        		  show_map();
        	  }
        	  else if (c == '4'){ //view inventory
        		  gs.prev_state = STATE_COASTLINE;
        		  gs.state = STATE_INVENTORY;
        		  show_inventory();
        	  }
        	  else if (c == '5'){ //travel
        		  gs.state = STATE_TRAVEL;
        		  show_travel_menu();
        	  }
        	  else if (c == '6') {
        	      gs.prev_state = gs.state;
        	      gs.state = STATE_INVENTORY;
        	      gs.day++;
        	      night_event();
        	  }
        	      else if (c == '7') {
        	      gs.prev_state = gs.state;
        	      gs.state = STATE_INVENTORY;
        	      show_notes();
        	  }
        	  break;
          case STATE_TOWER:
              if (c == '1') {
                  gs.prev_state = STATE_TOWER;
                  gs.state = STATE_PUZZLE;
                  puzzle_radio();
              } else if (c == '2') {
                  gs.prev_state = STATE_TOWER;
                  gs.state = STATE_INVENTORY;
                  show_map();
              } else if (c == '3') {
                  gs.prev_state = STATE_TOWER;
                  gs.state = STATE_INVENTORY;
                  show_inventory();
              } else if (c == '4') {
                  gs.prev_state = STATE_TOWER;
                  gs.state = STATE_INVENTORY;
                  show_notes();
              } else if (c == '5') {
                  gs.state = STATE_TRAVEL;
                  show_travel_menu();
              }
              break;

          case STATE_BUNKER:
              if (c == '1') {
                  gs.prev_state = STATE_BUNKER;
                  gs.state = STATE_PUZZLE;
                  puzzle_bunker();

              } else if (c == '2') {
                  gs.prev_state = STATE_BUNKER;
                  gs.state = STATE_INVENTORY;
                  show_map();
              } else if (c == '3') {
                  gs.prev_state = STATE_BUNKER;
                  gs.state = STATE_INVENTORY;
                  show_inventory();
              } else if (c == '4') {
                  gs.prev_state = STATE_BUNKER;
                  gs.state = STATE_INVENTORY;
                  show_notes();
              } else if (c == '5') {
                  gs.state = STATE_TRAVEL;
                  show_travel_menu();
              }
              break;
          case STATE_PUZZLE:
              if (gs.active_puzzle == 0){ //radio tower puzzle
                  // Echo the character back so player can see what they typed
                  char echo[2] = {c, '\0'};
                  UART_print(echo);
                  // Store input
                  gs.puzzle_input[gs.puzzle_step] = c;
                  gs.puzzle_step++;
                  if (gs.puzzle_step >= 4) {
                      UART_print("\r\n");
                      // Check answer
                      if ((gs.puzzle_input[0] == 'B' || gs.puzzle_input[0] == 'b') &&
                          (gs.puzzle_input[1] == 'R' || gs.puzzle_input[1] == 'r') &&
                          (gs.puzzle_input[2] == 'Y' || gs.puzzle_input[2] == 'y') &&
                          (gs.puzzle_input[3] == 'G' || gs.puzzle_input[3] == 'g')) {
                          // Correct
                          gs.puzzles_solved |= 0x01;
                          UART_print("\r\nThe radio crackles to life!\r\n");
                          UART_print("A voice cuts through the static:\r\n");
                          UART_print("\"...survivor signal received... coordinates locked...\"\r\n");
                          UART_print("\"...rescue team en route... hold your position...\"\r\n");
                          UART_print("\r\nYou solved the Radio puzzle!\r\n");
                      }
                      else {
                          // Wrong
                          UART_print("\r\nSparks fly. Wrong sequence.\r\n");
                          gs.health -= 10;
                          if (gs.health <= 0) gs.health = 0;
                          UART_print("Health decreased.\r\n");
                      }
                      gs.puzzle_step = 0;
                      memset(gs.puzzle_input, 0, sizeof(gs.puzzle_input));
                      gs.state = STATE_TOWER;
                      UART_print("\r\nPress any key to continue...\r\n");
                      gs.state = STATE_INVENTORY;
                      gs.prev_state = STATE_TOWER;
                      check_death_conditions();
                  }
              }
              else if (gs.active_puzzle == 1){ //cave puzzle
                  char echo[2] = {c, '\0'};
                  UART_print(echo);
                  UART_print("\r\n");
                  const char correct[3] = {'B', 'A', 'C'};
                  if (c == correct[gs.puzzle_step] || c == correct[gs.puzzle_step] + 32) {
                      // Correct choice
                      gs.puzzle_step++;
                      if (gs.puzzle_step == 1) UART_print("You clear pile B. The passage opens slightly.\r\n");
                      if (gs.puzzle_step == 2) UART_print("You heave pile A aside. Almost through.\r\n");
                      if (gs.puzzle_step == 3) {
                          // Solved
                          gs.puzzles_solved |= 0x02;
                          UART_print("You carefully remove pile C. The passage is clear!\r\n");
                          UART_print("Fresh air rushes in. You crawl through to the other side.\r\n");
                          UART_print("You found an underground chamber with supplies.\r\n");
                          UART_print("\r\nYou solved the Cave puzzle!\r\n");
                          gs.puzzle_step = 0;
                          gs.active_puzzle = 0;
                          gs.state = STATE_INVENTORY;
                          gs.prev_state = STATE_CAVE;
                          UART_print("\r\nPress any key to continue...\r\n");
                      } else {
                          // Prompt next move
                          UART_print("Which pile next? (A/B/C):\r\n> ");
                      }
                  } else {
                      // Wrong
                      gs.fear += 10;
                      if (gs.fear > 100) gs.fear = 100;
                      gs.puzzle_step = 0; // reset sequence
                      if (gs.difficulty == 0) {
                          UART_print("Wrong choice! The debris shifts dangerously.\r\n");
                          UART_print("Fear increased. Try again from the start.\r\n");
                          UART_print("Hint: Start with B.\r\n");
                      } else {
                          UART_print("The debris collapses. You scramble back.\r\n");
                          UART_print("Fear increased. Try again from the start.\r\n");
                      }
                      UART_print("Which pile do you move first? (A/B/C):\r\n> ");
                  }
                  check_death_conditions();
              }
              else if (gs.active_puzzle == 2){ // Bunker puzzle
                  char echo[2] = {c, '\0'};
                  UART_print(echo);
                  UART_print("\r\n");
                  const char correct[3] = {'3', '1', '2'};
                  if (c == correct[gs.puzzle_step]) {
                      gs.puzzle_step++;
                      if (gs.puzzle_step == 1) {
                          UART_print("You prime the fuel line. It sputters and catches.\r\n");
                          UART_print("Next step (1/2/3):\r\n> ");
                      }
                      else if (gs.puzzle_step == 2) {
                          UART_print("You connect the battery. The generator hums weakly.\r\n");
                          UART_print("Next step (1/2/3):\r\n> ");
                      }
                      else if (gs.puzzle_step == 3) {
                          // Solved
                          gs.puzzles_solved |= 0x04;
                          UART_print("You route the wiring. The generator roars to life!\r\n");
                          UART_print("Lights flicker on throughout the bunker.\r\n");
                          UART_print("A radio console lights up in the corner.\r\n");
                          UART_print("An automated distress beacon activates.\r\n\r\n");
                          UART_print("You solved the Bunker puzzle!\r\n");
                          gs.puzzle_step = 0;
                          gs.active_puzzle = 0;
                          gs.state = STATE_INVENTORY;
                          gs.prev_state = STATE_BUNKER;
                          UART_print("\r\nPress any key to continue...\r\n");
                      }
                  }
                  else {
                      // Wrong choice
                      gs.health -= 10;
                      gs.fear += 10;
                      if (gs.health < 0) gs.health = 0;
                      if (gs.fear > 100) gs.fear = 100;
                      gs.puzzle_step = 0;
                      if (c == '1' || c == '2') {
                          UART_print("Sparks fly as the system shorts out.\r\n");
                          UART_print("You were thrown back by the surge.\r\n");
                      }
                      else {
                          UART_print("Something goes wrong. The generator sputters and dies.\r\n");
                      }
                      if (gs.difficulty == 0) {
                          UART_print("Health and fear affected. Try again.\r\n");
                          UART_print("Hint: Prime the fuel line first.\r\n");
                      }
                      else {
                          UART_print("Health and fear affected. Try again.\r\n");
                      }
                      UART_print("Choose your first step (1/2/3):\r\n> ");
                      check_death_conditions();
                  }
              }
              break;

          default:
              break;
      }

  } /* Game engine END WHILE */

  // Game over
  while(1);
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
/* If UART2 has received a byte from the user terminal this function
 * will be called.
 * dataAvail is used to indicate that the rx_buff has received new data and can be checked.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  HAL_UART_Receive_IT(&huart2, (uint8_t *)rx_buff, 1); //You need to toggle a breakpoint on this line!
  dataAvail=1;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
