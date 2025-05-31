#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700
#define MAX_TRIES 6
#define MAX_WORD_LENGTH 50
#define MAX_HINT_LENGTH 150
#define MAX_CATEGORIES 10
#define MAX_WORDS_PER_CATEGORY 50
#define MAX_NAME_LENGTH 50
#define HIGH_SCORES_FILE "hangman_scores.txt"
#define MAX_HIGH_SCORES 12
#define CATEGORIES_DIR "categories/"

#define PINK_MEDIUM (Color){255, 105, 180, 255}  
#define PINK_DARK (Color){199, 21, 133, 255}     
#define PINK_ACCENT (Color){255, 20, 147, 255} 
#define WHITE_PINK (Color){255, 240, 245, 255}

typedef enum {
    SCREEN_MENU,
    SCREEN_GAME,
    SCREEN_HIGH_SCORES,
    SCREEN_INSTRUCTIONS,
    SCREEN_CATEGORY_SELECT,
    SCREEN_NAME_INPUT,
    SCREEN_GAME_OVER
} ScreenState;

typedef struct {
    char name[MAX_NAME_LENGTH];
    int score;
    time_t date;
} HighScore;

typedef struct {
    char word[MAX_WORD_LENGTH];
    char hint[MAX_HINT_LENGTH];
} Word;

typedef struct {
    char name[MAX_WORD_LENGTH];
    Word words[MAX_WORDS_PER_CATEGORY];
    int wordCount;
} Category;

typedef struct {
    ScreenState currentScreen;
    char playerName[MAX_NAME_LENGTH];
    int selectedCategory;
    int selectedWord;
    char currentWord[MAX_WORD_LENGTH];
    char currentHint[MAX_HINT_LENGTH];
    char guessedLetters[27];
    int guessCount;
    int wrongGuesses;
    time_t startTime;
    int gameWon;
    int finalScore;
    float animationTimer;
    int buttonHover;
    char inputBuffer[MAX_NAME_LENGTH];
    int inputActive;
    int scrollOffset;
    bool shouldExit;
} GameState;

Category categories[MAX_CATEGORIES];
int categoryCount = 0;

int compareScores(const void *a, const void *b) {
    return ((HighScore*)b)->score - ((HighScore*)a)->score;
}
void saveScore(char* playerName, int score) {
    HighScore scores[MAX_HIGH_SCORES];
    int numScores = 0;
    int existingPlayerIndex = -1;
    
    FILE *file = fopen(HIGH_SCORES_FILE, "r");
    
    if (file != NULL) {
        while (numScores < MAX_HIGH_SCORES && 
               fscanf(file, "%49[^,],%d,%ld\n", 
                      scores[numScores].name, 
                      &scores[numScores].score, 
                      &scores[numScores].date) == 3) {
            
            // Verifică dacă jucătorul există deja
            if (strcasecmp(scores[numScores].name, playerName) == 0) {
                existingPlayerIndex = numScores;
            }
            
            numScores++;
        }
        fclose(file);
    }
    
    if (existingPlayerIndex != -1) {
        // Jucătorul există
        scores[existingPlayerIndex].score += score;
        scores[existingPlayerIndex].date = time(NULL);
    } else {
        // Jucător nou
        if (numScores < MAX_HIGH_SCORES) {
            strcpy(scores[numScores].name, playerName);
            scores[numScores].score = score;
            scores[numScores].date = time(NULL);
            numScores++;
        } else {
            int minScoreIndex = 0;
            for (int i = 1; i < numScores; i++) {
                if (scores[i].score < scores[minScoreIndex].score) {
                    minScoreIndex = i;
                }
            }
            
            if (score > scores[minScoreIndex].score) {
                strcpy(scores[minScoreIndex].name, playerName);
                scores[minScoreIndex].score = score;
                scores[minScoreIndex].date = time(NULL);
            }
        }
    }
    
    qsort(scores, numScores, sizeof(HighScore), compareScores);
    
    file = fopen(HIGH_SCORES_FILE, "w");
    if (file != NULL) {
        for (int i = 0; i < numScores; i++) {
            fprintf(file, "%s,%d,%ld\n", 
                    scores[i].name, 
                    scores[i].score, 
                    scores[i].date);
        }
        fclose(file);
    }
}

int calculateScore(int wrongGuesses, int timeSpent, int difficultyLevel) {
    int baseScore = 1000;
    int penaltyForWrongGuesses = wrongGuesses * 100;
    int penaltyForTime = timeSpent / 5;
    int bonusForDifficulty = difficultyLevel * 200;
    
    int score = baseScore - penaltyForWrongGuesses - penaltyForTime + bonusForDifficulty;
    return score > 0 ? score : 0;
}

void CreateExampleCategories() {
  system("mkdir -p categories");
    }

void LoadCategoriesFromFiles() {
    categoryCount = 0;
    
    FILE *testFile = fopen("categories/animale.txt", "r");
    if (!testFile) {
        CreateExampleCategories();
    } else {
        fclose(testFile);
    }
    
    const char* categoryFiles[] = {
        "categories/animale.txt",
        "categories/filme.txt", 
        "categories/tari.txt",
        "categories/mancare.txt",
        "categories/tehnologie.txt",
        "categories/sporturi.txt",
        "categories/culori.txt",
        "categories/meserii.txt",
        "categories/natura.txt",
        "categories/muzica.txt"
    };
    
    const char* categoryNames[] = {
        "Animale",
        "Filme",
        "Tari", 
        "Mancare",
        "Tehnologie",
        "Sporturi",
        "Culori",
        "Meserii",
        "Natura",
        "Muzica"
    };
    
    for (int i = 0; i < 10 && categoryCount < MAX_CATEGORIES; i++) {
        FILE *file = fopen(categoryFiles[i], "r");
        if (file) {
            strcpy(categories[categoryCount].name, categoryNames[i]);
            categories[categoryCount].wordCount = 0;
            
            char line[300];
            while (fgets(line, sizeof(line), file) && 
                   categories[categoryCount].wordCount < MAX_WORDS_PER_CATEGORY) {
                
                line[strcspn(line, "\n")] = 0;
                
                char *word = strtok(line, "|");
                char *hint = strtok(NULL, "|");
                
                if (word && hint) {
                    for (int j = 0; word[j]; j++) {
                        word[j] = tolower(word[j]);
                    }
                    
                    strcpy(categories[categoryCount].words[categories[categoryCount].wordCount].word, word);
                    strcpy(categories[categoryCount].words[categories[categoryCount].wordCount].hint, hint);
                    categories[categoryCount].wordCount++;
                }
            }
            
            fclose(file);
            
            if (categories[categoryCount].wordCount > 0) {
                categoryCount++;
            }
        }
    }
}

bool DrawButton(Rectangle rect, const char* text, int fontSize, Color baseColor, Color textColor) {
    bool isHovered = CheckCollisionPointRec(GetMousePosition(), rect);
    bool isClicked = isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    
    Color buttonColor = baseColor;
    if (isHovered) {
        buttonColor = (Color){
            (baseColor.r > 20) ? baseColor.r - 20 : 0,
            (baseColor.g > 20) ? baseColor.g - 20 : 0, 
            (baseColor.b > 20) ? baseColor.b - 20 : 0,
            baseColor.a
        };
    }
    
    DrawRectangleRounded((Rectangle){rect.x + 3, rect.y + 3, rect.width, rect.height}, 0.3f, 8, (Color){0, 0, 0, 50});
    
    // Buton principal
    DrawRectangleRounded(rect, 0.3f, 8, buttonColor);
    DrawRectangleRoundedLines(rect, 0.3f, 8, PINK_DARK);
    
    // Text centrat
    Vector2 textSize = MeasureTextEx(GetFontDefault(), text, fontSize, 1);
    Vector2 textPos = {
        rect.x + (rect.width - textSize.x) / 2,
        rect.y + (rect.height - textSize.y) / 2
    };
    DrawTextEx(GetFontDefault(), text, textPos, fontSize, 1, textColor);
    
    return isClicked;
}

void DrawHangman(int wrongGuesses, Vector2 position) {
    float scale = 1.5f;
    Color gallowsColor = BROWN;
    Color ropeColor = WHITE;
    Color personColor = PINK_DARK;
    
    // Baza spânzurătorii
    DrawRectangle(position.x, position.y + 150 * scale, 120 * scale, 10 * scale, gallowsColor);
    
    // Stâlpul vertical
    DrawRectangle(position.x + 20 * scale, position.y, 8 * scale, 160 * scale, gallowsColor);
    
    // Bara orizontală
    DrawRectangle(position.x + 20 * scale, position.y, 80 * scale, 8 * scale, gallowsColor);
    
    // Frânghia
    DrawRectangle(position.x + 90 * scale, position.y + 8 * scale, 3 * scale, 40 * scale, ropeColor);
    
    if (wrongGuesses >= 1) {
    // Cap
    DrawCircle(position.x + 92 * scale, position.y + 52 * scale, 10 * scale, personColor);
    DrawCircleLines(position.x + 92 * scale, position.y + 52 * scale, 10 * scale, PINK_DARK);
    }
    if (wrongGuesses >= 2) {
        // Corp
        DrawRectangle(position.x + 90 * scale, position.y + 62 * scale, 4 * scale, 35 * scale, personColor);
	}
    if (wrongGuesses >= 3) {
    // Brațul stâng
    DrawLineEx((Vector2){position.x + 92 * scale, position.y + 70 * scale}, 
               (Vector2){position.x + 75 * scale, position.y + 85 * scale}, 
               4 * scale, personColor);
    }
    if (wrongGuesses >= 4) {
    // Brațul drept
    DrawLineEx((Vector2){position.x + 92 * scale, position.y + 70 * scale}, 
               (Vector2){position.x + 109 * scale, position.y + 85 * scale}, 
               4 * scale, personColor);
    }
    if (wrongGuesses >= 5) {
    // Piciorul stâng
    DrawLineEx((Vector2){position.x + 92 * scale, position.y + 97 * scale}, 
               (Vector2){position.x + 80 * scale, position.y + 130 * scale}, 
               4 * scale, personColor);
    }
    if (wrongGuesses >= 6) {
    // Piciorul drept  
    DrawLineEx((Vector2){position.x + 92 * scale, position.y + 97 * scale}, 
               (Vector2){position.x + 104 * scale, position.y + 130 * scale}, 
               4 * scale, personColor);
    }
}

bool IsWordGuessed(const char* word, const char* guessedLetters) {
    for (int i = 0; i < strlen(word); i++) {
        if (word[i] != '-' && word[i] != ' ' && !strchr(guessedLetters, word[i])) {
            return false;
        }
    }
    return true;
}

void DrawWord(const char* word, const char* guessedLetters, Vector2 position, int fontSize) {
    char displayWord[MAX_WORD_LENGTH * 2] = {0};
    
    for (int i = 0; i < strlen(word); i++) {
        if (strchr(guessedLetters, word[i]) != NULL || word[i] == '-' || word[i] == ' ') {
            strncat(displayWord, &word[i], 1);
        } else {
            strcat(displayWord, "_");
        }
        strcat(displayWord, " ");
    }
    
    Vector2 textSize = MeasureTextEx(GetFontDefault(), displayWord, fontSize, 1);
    Vector2 centeredPos = {
        position.x - textSize.x / 2,
        position.y
    };
    
    DrawTextEx(GetFontDefault(), displayWord, centeredPos, fontSize, 1, PINK_DARK);
}

// Funcție pentru încărcarea și afișarea clasamentului
void DrawHighScores() {
    HighScore scores[MAX_HIGH_SCORES];
    int numScores = 0;
    FILE *file = fopen(HIGH_SCORES_FILE, "r");
    
    if (file != NULL) {
        while (numScores < MAX_HIGH_SCORES && 
               fscanf(file, "%49[^,],%d,%ld\n", 
                      scores[numScores].name, 
                      &scores[numScores].score, 
                      &scores[numScores].date) == 3) {
            numScores++;
        }
        fclose(file);
    }
    
    DrawTextEx(GetFontDefault(), "CLASAMENT", (Vector2){SCREEN_WIDTH/2 - 110, 80}, 42, 1, PINK_ACCENT);
    
    if (numScores == 0) {
        DrawTextEx(GetFontDefault(), "Nu exista scoruri salvate", (Vector2){SCREEN_WIDTH/2 - 120, 200}, 21, 1, PINK_MEDIUM);
    } else {
        for (int i = 0; i < numScores; i++) {
            char scoreText[100];
            snprintf(scoreText, sizeof(scoreText), "%d. %s - %d puncte", i + 1, scores[i].name, scores[i].score);
            DrawTextEx(GetFontDefault(), scoreText, (Vector2){SCREEN_WIDTH/2 - 110, 150 + i * 30}, 21, 1, PINK_DARK);
        }
    }
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Spanzuratoarea");
    SetTargetFPS(60);
    
    srand(time(NULL));
    
    LoadCategoriesFromFiles();
    
    GameState gameState = {0};
    gameState.currentScreen = SCREEN_MENU;
    gameState.buttonHover = -1;
    strcpy(gameState.inputBuffer, "");
    gameState.shouldExit= false;
    
    while (!WindowShouldClose() && !gameState.shouldExit) {
        gameState.animationTimer += GetFrameTime();
        
        // Input pentru numele jucătorului
        if (gameState.currentScreen == SCREEN_NAME_INPUT && gameState.inputActive) {
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125) && (strlen(gameState.inputBuffer) < MAX_NAME_LENGTH - 1)) {
                    int len = strlen(gameState.inputBuffer);
                    gameState.inputBuffer[len] = (char)key;
                    gameState.inputBuffer[len + 1] = '\0';
                }
                key = GetCharPressed();
            }
            
            if (IsKeyPressed(KEY_BACKSPACE)) {
                int len = strlen(gameState.inputBuffer);
                if (len > 0) gameState.inputBuffer[len - 1] = '\0';
            }
        }
        
        // Input pentru joc
        if (gameState.currentScreen == SCREEN_GAME) {
            int key = GetKeyPressed();
            if (key >= KEY_A && key <= KEY_Z) {
                char letter = 'a' + (key - KEY_A);
                
                // Verifică dacă litera nu a fost deja încercată
                if (!strchr(gameState.guessedLetters, letter)) {
                    gameState.guessedLetters[gameState.guessCount++] = letter;
                    gameState.guessedLetters[gameState.guessCount] = '\0';
                    
                    // Verifică dacă litera este în cuvânt
                    if (!strchr(gameState.currentWord, letter)) {
                        gameState.wrongGuesses++;
                    }
                    
                    // Verifică dacă jocul s-a terminat
                    if (gameState.wrongGuesses >= MAX_TRIES) {
                        gameState.gameWon = 0;
                        gameState.currentScreen = SCREEN_GAME_OVER;
                    } else if (IsWordGuessed(gameState.currentWord, gameState.guessedLetters)) {
                        gameState.gameWon = 1;
                        time_t endTime = time(NULL);
                        int timeSpent = (int)difftime(endTime, gameState.startTime);
                        int difficultyLevel = strlen(gameState.currentWord) / 3;
                        gameState.finalScore = calculateScore(gameState.wrongGuesses, timeSpent, difficultyLevel);
                        saveScore(gameState.playerName, gameState.finalScore);
                        gameState.currentScreen = SCREEN_GAME_OVER;
                    }
                }
            }
        }
        
        BeginDrawing();
        
        DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE_PINK, PINK_ACCENT);
        
        // Desenează stelute decorative
        for (int i = 0; i < 20; i++) {
            int x = (i * 137) % SCREEN_WIDTH;
            int y = (i * 241) % SCREEN_HEIGHT;
            float alpha = 0.3f + 0.4f * sin(gameState.animationTimer * 2 + i);
            DrawCircle(x, y, 2, (Color){255, 255, 255, (unsigned char)(alpha * 255)});
	    }
        
        switch (gameState.currentScreen) {
            case SCREEN_MENU: {
                // Titlu principal
                const char* title = "SPANZURATOAREA";
                int titleSize = 60;
                Vector2 titleMeasure = MeasureTextEx(GetFontDefault(), title, titleSize, 1);
                Vector2 titlePos = {(SCREEN_WIDTH - titleMeasure.x) / 2, 100};
                
                // Titlu
                DrawTextEx(GetFontDefault(), title, titlePos, titleSize, 1, PINK_DARK);
                
                
                // Butoane
                if (DrawButton((Rectangle){SCREEN_WIDTH/2 - 100, 250, 200, 60}, "Joc Nou", 28, PINK_ACCENT, WHITE)) {
                    gameState.currentScreen = SCREEN_NAME_INPUT;
                    gameState.inputActive = 1;
                    strcpy(gameState.inputBuffer, "");
                }
                
                if (DrawButton((Rectangle){SCREEN_WIDTH/2 - 100, 330, 200, 60}, "Clasament", 28, PINK_ACCENT, WHITE)) {
                    gameState.currentScreen = SCREEN_HIGH_SCORES;
                }
                
                if (DrawButton((Rectangle){SCREEN_WIDTH/2 - 100, 410, 200, 60}, "Instructiuni", 28, PINK_ACCENT, WHITE)) {
                    gameState.currentScreen = SCREEN_INSTRUCTIONS;
                }
                
                if (DrawButton((Rectangle){SCREEN_WIDTH/2 - 100, 490, 200, 60}, "Iesire", 30, PINK_DARK, WHITE)) {
                    gameState.shouldExit = true;
                }
                break;
            }
            
            case SCREEN_NAME_INPUT: {
                DrawTextEx(GetFontDefault(), "Introdu numele tau:", (Vector2){SCREEN_WIDTH/2 - 125, 250}, 30, 1, PINK_DARK);
                
                // Câmp de input
                Rectangle inputRect = {SCREEN_WIDTH/2 - 150, 300, 300, 40};
                DrawRectangleRounded(inputRect, 0.3f, 8, WHITE);
                DrawRectangleRoundedLines(inputRect, 0.3f, 8, PINK_MEDIUM);
                
                DrawTextEx(GetFontDefault(), gameState.inputBuffer, (Vector2){inputRect.x + 10, inputRect.y + 10}, 20, 1, PINK_DARK);
                
                // Cursor care clipește
                if (gameState.inputActive && ((int)(gameState.animationTimer * 2) % 2)) {
                    Vector2 textSize = MeasureTextEx(GetFontDefault(), gameState.inputBuffer, 20, 1);
                    DrawRectangle(inputRect.x + 10 + textSize.x, inputRect.y + 8, 2, 24, PINK_DARK);
                }
                
                if (DrawButton((Rectangle){SCREEN_WIDTH/2 - 60, 380, 120, 40}, "Continua", 20, PINK_ACCENT, WHITE)) {
                    if (strlen(gameState.inputBuffer) > 0) {
                        strcpy(gameState.playerName, gameState.inputBuffer);
                        gameState.currentScreen = SCREEN_CATEGORY_SELECT;
                        gameState.inputActive = 0;
                    }
                }
                
                if (DrawButton((Rectangle){SCREEN_WIDTH/2 - 60, 440, 120, 40}, "Inapoi", 20, WHITE_PINK, PINK_DARK)) {
                    gameState.currentScreen = SCREEN_MENU;
                    gameState.inputActive = 0;
                }
                break;
            }
            
            case SCREEN_CATEGORY_SELECT: {
                DrawTextEx(GetFontDefault(), "Alege o categorie:", (Vector2){SCREEN_WIDTH/2 - 160, 80}, 40, 1, PINK_DARK);
                
                // Afișează categoriile disponibile
                int buttonWidth = 200;
                int buttonHeight = 50;
                int buttonsPerRow = 2;
                int startY = 180;
                
                for (int i = 0; i < categoryCount; i++) {
                    int row = i / buttonsPerRow;
                    int col = i % buttonsPerRow;
                    
                    int x = SCREEN_WIDTH/2 - (buttonsPerRow * (buttonWidth + 20)) / 2 + col * (buttonWidth + 20);
                    int y = startY + row * (buttonHeight + 20);
                    
                    Rectangle buttonRect = {x, y, buttonWidth, buttonHeight};
                    
                    char buttonText[100];
                    //snprintf(buttonText, sizeof(buttonText), "%s (%d)", categories[i].name, categories[i].wordCount);
		    snprintf(buttonText, sizeof(buttonText), "%s", categories[i].name);

                    
                    if (DrawButton(buttonRect, buttonText, 20, PINK_ACCENT, WHITE)) {
                        gameState.selectedCategory = i;
                        gameState.selectedWord = rand() % categories[i].wordCount;
                        
                        strcpy(gameState.currentWord, categories[i].words[gameState.selectedWord].word);
                        strcpy(gameState.currentHint, categories[i].words[gameState.selectedWord].hint);
                        
                        // Resetează starea jocului
                        memset(gameState.guessedLetters, 0, sizeof(gameState.guessedLetters));
                       gameState.guessCount = 0;
                       gameState.wrongGuesses = 0;
                       gameState.gameWon = 0;
                       gameState.startTime = time(NULL);
                       gameState.currentScreen = SCREEN_GAME;
                   }
               }
               
               if (DrawButton((Rectangle){SCREEN_WIDTH/2 - 60, startY + ((categoryCount + buttonsPerRow - 1) / buttonsPerRow) * 70 + 40, 120, 40}, "Inapoi", 20, WHITE_PINK, PINK_DARK)) {
                   gameState.currentScreen = SCREEN_NAME_INPUT;
               }
               break;
           }
           
           case SCREEN_GAME: {
               // Titlul categoriei
               char categoryTitle[100];
               snprintf(categoryTitle, sizeof(categoryTitle), "Categoria: %s", categories[gameState.selectedCategory].name);
               Vector2 categorySize = MeasureTextEx(GetFontDefault(), categoryTitle, 20, 1);
               DrawTextEx(GetFontDefault(), categoryTitle, (Vector2){SCREEN_WIDTH/2 - categorySize.x/2, 50}, 25, 1, PINK_ACCENT);
               
               // Indiciul
               char hintText[200];
               snprintf(hintText, sizeof(hintText), "Indiciu: %s", gameState.currentHint);
               Vector2 hintSize = MeasureTextEx(GetFontDefault(), hintText, 18, 1);
               DrawTextEx(GetFontDefault(), hintText, (Vector2){SCREEN_WIDTH/2 - hintSize.x/2, 80}, 20, 1, PINK_DARK);
               
               DrawHangman(gameState.wrongGuesses, (Vector2){50, 150});
               
               DrawWord(gameState.currentWord, gameState.guessedLetters, (Vector2){SCREEN_WIDTH/2, 400}, 36);
               
               // Afișează literele încercate
               DrawTextEx(GetFontDefault(), "Litere incercate:", (Vector2){SCREEN_WIDTH/2 + 200, 200}, 20, 1, PINK_DARK);
               
               char wrongLetters[50] = {0};
               for (int i = 0; i < gameState.guessCount; i++) {
                   if (!strchr(gameState.currentWord, gameState.guessedLetters[i])) {
                       char temp[3] = {gameState.guessedLetters[i], ' ', '\0'};
                       strcat(wrongLetters, temp);
                   }
               }
               
               DrawTextEx(GetFontDefault(), wrongLetters, (Vector2){SCREEN_WIDTH/2 + 200, 230}, 20, 1, WHITE_PINK);
               
               // Afișează numărul de încercări rămase
               char triesText[50];
               snprintf(triesText, sizeof(triesText), "Incercari ramase: %d", MAX_TRIES - gameState.wrongGuesses);
               DrawTextEx(GetFontDefault(), triesText, (Vector2){SCREEN_WIDTH/2 + 200, 270}, 20, 1, PINK_DARK);
               
               DrawTextEx(GetFontDefault(), "Apasa o litera pentru a ghici!", (Vector2){SCREEN_WIDTH/2 - 120, 500}, 18, 1, WHITE_PINK);
               
               if (DrawButton((Rectangle){SCREEN_WIDTH - 120, 10, 100, 40}, "Meniu", 18, WHITE_PINK, PINK_DARK)) {
                   gameState.currentScreen = SCREEN_MENU;
               }
               break;
           }
           
           case SCREEN_GAME_OVER: {
               if (gameState.gameWon) {
                   const char* winText = "FELICITARI!";
                   Vector2 winSize = MeasureTextEx(GetFontDefault(), winText, 48, 1);
                   DrawTextEx(GetFontDefault(), winText, (Vector2){SCREEN_WIDTH/2 - winSize.x/2, 150}, 48, 1, PINK_ACCENT);
                   
                   DrawTextEx(GetFontDefault(), "Ai ghicit cuvantul!", (Vector2){SCREEN_WIDTH/2 - 100, 220}, 24, 1, PINK_DARK);
                   
                   char scoreText[100];
                   snprintf(scoreText, sizeof(scoreText), "Scorul tau: %d puncte", gameState.finalScore);
                   Vector2 scoreSize = MeasureTextEx(GetFontDefault(), scoreText, 20, 1);
                   DrawTextEx(GetFontDefault(), scoreText, (Vector2){SCREEN_WIDTH/2 - scoreSize.x/2, 270}, 20, 1, WHITE_PINK);
               } else {
                   const char* loseText = "AI PIERDUT!";
                   Vector2 loseSize = MeasureTextEx(GetFontDefault(), loseText, 48, 1);
                   DrawTextEx(GetFontDefault(), loseText, (Vector2){SCREEN_WIDTH/2 - loseSize.x/2, 150}, 48, 1, PINK_DARK);
                   
                   char correctText[100];
                   snprintf(correctText, sizeof(correctText), "Cuvantul era: %s", gameState.currentWord);
                   Vector2 correctSize = MeasureTextEx(GetFontDefault(), correctText, 20, 1);
                   DrawTextEx(GetFontDefault(), correctText, (Vector2){SCREEN_WIDTH/2 - correctSize.x/2, 200}, 20, 1, PINK_ACCENT);
                   
                   DrawHangman(MAX_TRIES, (Vector2){SCREEN_WIDTH/2 - 60, 230});
               }
               
               if (DrawButton((Rectangle){SCREEN_WIDTH/2 - 120, 500, 100, 50}, "Joc Nou", 20, PINK_MEDIUM, WHITE)) {
                   gameState.currentScreen = SCREEN_CATEGORY_SELECT;
               }
               
               if (DrawButton((Rectangle){SCREEN_WIDTH/2 + 20, 500, 100, 50}, "Meniu", 20, WHITE_PINK, PINK_DARK)) {
                   gameState.currentScreen = SCREEN_MENU;
               }
               break;
           }
           
           case SCREEN_HIGH_SCORES: {
               DrawHighScores();
               
               if (DrawButton((Rectangle){SCREEN_WIDTH/2 - 60, 550, 120, 50}, "Inapoi", 20, PINK_MEDIUM, WHITE)) {
                   gameState.currentScreen = SCREEN_MENU;
               }
               break;
           }
           
           case SCREEN_INSTRUCTIONS: {
               DrawTextEx(GetFontDefault(), "INSTRUCTIUNI", (Vector2){SCREEN_WIDTH/2 - 130, 80}, 42, 1, PINK_ACCENT);
               
               const char* instructions[] = {
                   "1. Introdu numele tau",
                   "2. Alege o categorie",
                   "3. Ghiceste cuvantul litera cu litera",
                   "4. Ai maxim 6 incercari gresite",
                   "5. Foloseste indiciul pentru ajutor",
                   "6. Cu cat ghicesti mai repede, cu atat",
                   "   primesti mai multe puncte!",
                   "",
                   "Apasa literele de la A-Z pentru a ghici.",
                   "Mult noroc!"
               };
               
               for (int i = 0; i < 10; i++) {
                   DrawTextEx(GetFontDefault(), instructions[i], (Vector2){SCREEN_WIDTH/2 - 200, 150 + i * 30}, 21, 1, PINK_DARK);
               }
               
               if (DrawButton((Rectangle){SCREEN_WIDTH/2 - 60, 520, 120, 50}, "Inapoi", 20, PINK_MEDIUM, WHITE)) {
                   gameState.currentScreen = SCREEN_MENU;
               }
               break;
           }
       }
       
       EndDrawing();
   }
   
   CloseWindow();
   return 0;
}
