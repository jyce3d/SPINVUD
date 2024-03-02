#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#include "stdio.h"
#include <time.h>

#define SCREEN_RATIO 2

#define KS_RIGHT  0
#define KS_LEFT  1
#define KS_UP 2
#define KS_SPACE 3

#define KS_KEYDOWN 1
#define KS_KEYUP 2

#define SIV_WND_WIDTH 1024
#define SIV_WND_HEIGHT 768

#define SIV_TXT_CENTER 1
#define SIV_TXT_LEFT 2
#define SIV_TEXT_FILLED 4

#define SIV_GAME 1
#define SIV_GAME_OVER 2
#define SIV_NEXT 4
#define SIV_STATUS_DEATH 8

#define MAX_MONSTRE 14

#define MAX_TIR_ALLOWED 10
/* SPACE INVUDERS - is a SHMUP inspired from space invaders (Taito 1978) */
/* jyce3d - 2024 */


/* Initialization des composants graphiques prinicipaux */

SDL_Window* pWindow = NULL;
TTF_Font *pFont =NULL;
SDL_Renderer* pRenderer = NULL;

// Initialisation des structure de données

typedef struct key_tag {
    int status;
} KeyStatus;
int status_score=0;
KeyStatus a_ks[4];

Uint32 last_time;
typedef struct monster_tag {
    int x,y;
    int status; // 0 dead, active 1, explose 1-> 2 , Explose 2-> 3, Explose3-> 4 
    int type ; // à sirena, 1 sconz, 2 flappy, 3 feliz
    int active; 
    int firing; // monstre en train de tirer
} Monster;

typedef struct active_monster_tag {
    Monster* monster;
    struct active_monster_tag * next;
} TActiveMonst;

typedef struct monster_tir_tag {
    int x,y;
    Monster *monster;
} TMonsterTir;

int g_monster_costume =0; // Costume des monstres, commes ils ont tous les même mouvemnt, on peut garer ça en globale hors de la structure
Monster m_sirena[MAX_MONSTRE];
Monster m_sconz[MAX_MONSTRE];
Monster m_flappy[MAX_MONSTRE];
Monster m_feliz[MAX_MONSTRE];
int gm_sens;
int g_cur_level = 0;
int status = SIV_GAME;
int tir_status;
int tir_x ;
int tir_y;
SDL_Surface* sprite_sfc = NULL;

int q=0;
int sfc_loaded=0;

/* Initialization des éléments graphiques */

SDL_Texture* pScene = NULL;

SDL_Texture* pSprite = NULL;
// Paramètres
int score=0;
int high_score =0;
int vie=3;

// Definition des mosaiques
// Pas de mosaique dans cet exemple

/* Définition des sprites */
SDL_Rect spr_player[1] = { {24/SCREEN_RATIO,268/SCREEN_RATIO, 90/SCREEN_RATIO, 120/SCREEN_RATIO } };
SDL_Rect spr_tir_monster[1] = { {133/SCREEN_RATIO,268/SCREEN_RATIO, 90/SCREEN_RATIO, 120/SCREEN_RATIO } };

SDL_Rect spr_player_dead[1] = { {40,416, (90-40),(498-416) }};


// Enemies
SDL_Rect spr_sirena[2]= { {1,1,125/SCREEN_RATIO,115/SCREEN_RATIO }, {132/SCREEN_RATIO,1,125/SCREEN_RATIO,115/SCREEN_RATIO} }; // Enemy 1
SDL_Rect spr_sconz[2] = { {255/SCREEN_RATIO,1,125/SCREEN_RATIO,115/SCREEN_RATIO}, {378/SCREEN_RATIO,1,121/SCREEN_RATIO ,115/SCREEN_RATIO} };  // Enemy 2
SDL_Rect spr_flappy[2] = { {1, 133/SCREEN_RATIO, 125/SCREEN_RATIO, 115/SCREEN_RATIO}, {132/SCREEN_RATIO,133/SCREEN_RATIO, 125/SCREEN_RATIO, 115/SCREEN_RATIO} };  // Enemy 3
SDL_Rect spr_feliz[2] = { {255/SCREEN_RATIO,133/SCREEN_RATIO, 125/SCREEN_RATIO,115/SCREEN_RATIO}, {373/SCREEN_RATIO,133/SCREEN_RATIO,125/SCREEN_RATIO, 115/SCREEN_RATIO} }; // Enemy 4
SDL_Rect spr_empty[1] = { {215/SCREEN_RATIO,260/SCREEN_RATIO, 75/SCREEN_RATIO,115/SCREEN_RATIO } }; // 0
SDL_Rect spr_expl_enemy[3] = { {132/SCREEN_RATIO,385/SCREEN_RATIO,125/SCREEN_RATIO,115/SCREEN_RATIO}, {255/SCREEN_RATIO,385/SCREEN_RATIO,125/SCREEN_RATIO,115/SCREEN_RATIO}, {0,385/SCREEN_RATIO,125/SCREEN_RATIO,115/SCREEN_RATIO} };
SDL_Rect spr_tir[1] = { {255/SCREEN_RATIO, 269/SCREEN_RATIO, 125/SCREEN_RATIO,115/SCREEN_RATIO } }; 

SDL_Rect spr_enemy_dest = {0, 0, 75/SCREEN_RATIO,115/SCREEN_RATIO};


SDL_Rect spr_player_dest = {0,0,90/SCREEN_RATIO,120/SCREEN_RATIO};

int play_x ;
int play_y ;
int dif_level=1 ;

// Gestion tir des monstres
// Max 4 tir autorisé (1 par rangée)
//int m_firing_cur_index;
int m_remaining_monsters;
TActiveMonst *m_first;

TMonsterTir m_monst_tir[MAX_TIR_ALLOWED];
int m_max_active_monster = 0;
void del_active_monster() {
    TActiveMonst *last;
    TActiveMonst * avant_dernier;
    //printf( "del_active_monster::Entering = %p   \n", m_first);
    while ( (last=m_first) !=NULL) {

        if (last->next ==NULL) {
            free(m_first);

            m_first =NULL;

        } else {

            while (last->next !=NULL) {
                avant_dernier = last;
                last = last->next;

            }

            free(last);
            avant_dernier->next =NULL;

        }
    }
    //printf ("del_active_monster::Outputing -> m_first= %p\n", m_first);
    m_max_active_monster=0;
}
void add_active_monster(Monster * m) {
    m_max_active_monster++;
    TActiveMonst *last;
    //printf ("add_active_monster::Entering for monster type =%d \n", m->type);

  //  printf("Add monster %d\n", m->type);
    if (m_first ==NULL) {
        m_first = (TActiveMonst*) malloc(sizeof(TActiveMonst));
        m_first->monster = m;
        m_first->next = NULL;
    } else {
        last = m_first;
        while ( (last->next) !=NULL) last = last->next;
        last->next = (TActiveMonst*) malloc(sizeof(TActiveMonst));
        last = last->next;
        last->monster = m;
        last->next = NULL;
    }
   // printf ("add_active_monster::Outputing\n");

}
Monster* retrieve_rand_col() {
    int max_idx;
    int i=0;
    TActiveMonst *last;
    //printf("retrieve_and_col:choix du  max active=%d \n", m_max_active_monster);
  
    max_idx = abs((rand()*m_max_active_monster+1) % m_max_active_monster);
    // si a déjà été choisi, je ne peux pas le rechoisir, je dois retirer

    last= m_first;
    while (i++<max_idx) last=last->next;   

    return last->monster;
}

Monster* choose_monster_fire(TMonsterTir* monster_tir) {
    Monster *selected = NULL;

    // si > 4 on choisit première passe
    // si 
    // sinon on prend ce que l'on trouve en bouclant de 0 à m_remaiing_monster-1

    for (int row=1;row<=4;row++) {
            for (int i=0;i<MAX_MONSTRE;i++) {
            switch(row) {
                case 1: //sirena
//                    printf("choose_monster_fire:sirena sconz-active=%d; flappy-active=%d ; feliz-active=%d\n ", m_sconz[i].active, m_flappy[i].active, m_feliz[i].active );
                    if (!m_sconz[i].active && !m_flappy[i].active && !m_feliz[i].active) {
                        if (m_sirena[i].active && !m_sirena[i].firing) add_active_monster(&m_sirena[i]);
                    }
                break;
                case 2: // le sconz
  //                  printf("choose_monster_fire:sconz  flappy-active=%d ; feliz-active=%d\n ",  m_flappy[i].active, m_feliz[i].active );

                    if ( !m_flappy[i].active && !m_feliz[i].active) {
                        if (m_sconz[i].active && !m_sconz[i].firing) add_active_monster(&m_sconz[i]);
                    }
                break;
                case 3: //flappy
    //                printf("choose_monster_fire:flappy-active=%d; flappy-firing=%d   feliz-active=%d\n ",   m_flappy[i].active, m_flappy[i].firing, m_feliz[i].active );

                    if (  !m_feliz[i].active) {
                        if (m_flappy[i].active && !m_flappy[i].firing) add_active_monster(&m_flappy[i]);
                    }

                break;
                case 4: //feliz
      //              printf("choose_monster_fire:feliz-active=%d   feliz-firinge=%d\n ",   m_feliz[i].active, m_feliz[i].firing );

                    if (m_feliz[i].active && !m_feliz[i].firing) add_active_monster(&m_feliz[i]);
                break;
            }
        }
        
    }
    if (m_first !=NULL) {
        selected =  retrieve_rand_col();
        if (selected !=NULL) {   
            selected->firing = 1;
            monster_tir->x= selected->x;
            monster_tir->y = selected->y;
            monster_tir->monster = selected;
        }
        del_active_monster();
    }

    return selected;

}

int is_tir_bottom(TMonsterTir *monster_tir) {
    if (monster_tir->monster !=NULL) {

        if (monster_tir->y >= play_y) {
            monster_tir->monster->firing = 0;
            monster_tir->monster=NULL;
            return 1;
        }
    }
    return 0;
}
void Initialize_ks() {
    
    a_ks[KS_RIGHT].status = KS_KEYUP;
    a_ks[KS_LEFT].status = KS_KEYUP;
    a_ks[KS_UP].status = KS_KEYUP;
    a_ks[KS_SPACE].status = KS_KEYUP;
}
void _init_sprite(Monster *m, int x, int y, int status, int type) {
    m->x = x;
    m->y = y;
    m->status = status;
    m->type = type;
    m->active = 1;
    m->firing = 0;

}
void initialize_level() {
    // Initialize le joueur

    play_x = 20;
    play_y = 616;
    gm_sens = 4;


    tir_status=0;
    tir_x=0 ;
    m_first  =NULL;
    m_remaining_monsters = (MAX_MONSTRE * 4) + 1;

 
    // initialize les monstres
    // sirena
    for (int i=0;i<MAX_MONSTRE;i++) {
        if (i>0) {
            _init_sprite(&m_sirena[i], m_sirena[i-1].x+80/SCREEN_RATIO, m_sirena[i-1].y, m_sirena[i-1].status,1);
            _init_sprite(&m_sconz[i], m_sconz[i-1].x+80/SCREEN_RATIO,m_sconz[i-1].y,m_sconz[i-1].status,2 );
            _init_sprite(&m_flappy[i],m_flappy[i-1].x+80/SCREEN_RATIO,m_flappy[i-1].y,m_flappy[i-1].status,3 );
            _init_sprite(&m_feliz[i],m_feliz[i-1].x+80/SCREEN_RATIO,m_feliz[i-1].y,m_feliz[i-1].status,4 );

        } else {
            _init_sprite(&m_sirena[i], 75/SCREEN_RATIO, 0, 1,1);
            _init_sprite(&m_sconz[i], 75/SCREEN_RATIO, 120/SCREEN_RATIO , 1,2);
            _init_sprite(&m_flappy[i], 75/SCREEN_RATIO, 240/SCREEN_RATIO , 1,3);
            _init_sprite(&m_feliz[i], 75/SCREEN_RATIO, 360/SCREEN_RATIO , 1,4);
        }

    }

    Initialize_ks();

}

void _display_monster(Monster* m)  {
    SDL_Rect dest_rect;
    dest_rect.w=75/SCREEN_RATIO;
    dest_rect.h=115/SCREEN_RATIO;
    dest_rect.x = m->x;
    dest_rect.y = m->y;
    SDL_Rect * sprite ;
    switch (m->type) {
        case 1:
            sprite = &spr_sirena[g_monster_costume];
        break;
        case 2:
            sprite = &spr_sconz[g_monster_costume];
        break;
        case 3:
            sprite = &spr_flappy[g_monster_costume];
        break;
        case 4:
            sprite = &spr_feliz[g_monster_costume];
        break;
    }

    switch (m->status ) {
            case 0:
                SDL_RenderCopy(pRenderer, pSprite, &spr_empty[0], &dest_rect);
            break;
            case 1:
                SDL_RenderCopy(pRenderer, pSprite, sprite, &dest_rect);
            break;
            case 2:
                SDL_RenderCopy(pRenderer, pSprite, &spr_expl_enemy[0], &dest_rect);
                if (SDL_GetTicks() - last_time >=350 )
                    m->status = 3;
            break;
            case 3:
                SDL_RenderCopy(pRenderer, pSprite, &spr_expl_enemy[1], &dest_rect);
                if (SDL_GetTicks() - last_time >=350 )
                    m->status = 4;
            break;
            case 4:
                SDL_RenderCopy(pRenderer, pSprite, &spr_expl_enemy[2], &dest_rect);
                if (SDL_GetTicks() - last_time >=350 ) {
                    m->status = 0; // Il a explosé;
                    if (m_remaining_monsters ==1) status = SIV_NEXT;
                }
            break;

        }

}
void display_enemy() {
    // Sirena
    for (int i=0;i<MAX_MONSTRE;i++) {
        _display_monster(&m_sirena[i]);
        _display_monster(&m_sconz[i]);
        _display_monster(&m_flappy[i]);
        _display_monster(&m_feliz[i]);
    }
}

void deplace_enemy() {
    int array_max = m_remaining_monsters >=MAX_TIR_ALLOWED? MAX_TIR_ALLOWED : m_remaining_monsters;
    SDL_Rect rct;
    // On teste les limites sur le premier monstre
    for (int i=0;i<MAX_MONSTRE;i++) {
            m_sirena[i].x+=gm_sens*dif_level;
            m_sconz[i].x+=gm_sens*dif_level;
            m_flappy[i].x+=gm_sens*dif_level;
            m_feliz[i].x+=gm_sens*dif_level;
    }
    if (SDL_GetTicks() - last_time >=500 ) {

        g_monster_costume++;
        g_monster_costume = g_monster_costume % 2;
        last_time = SDL_GetTicks();
        if ( (m_sirena[0].x <=70/SCREEN_RATIO ) || (m_sirena[0].x>=768/SCREEN_RATIO))
            gm_sens =-gm_sens;
        for (int i=0;i<MAX_MONSTRE;i++) {
            m_sirena[i].y+=dif_level;
            if (m_sirena[i].y>= play_y ) { 
                status= SIV_STATUS_DEATH;
                break; 
            }
            m_sconz[i].y+=dif_level;
            if (m_sconz[i].y>= play_y ) { 
                status= SIV_STATUS_DEATH;
                break; 
            }
            m_flappy[i].y+=dif_level;
            if (m_flappy[i].y>= play_y ) { 
                status= SIV_STATUS_DEATH;
                break; 
            }
            m_feliz[i].y+=dif_level;
            if (m_feliz[i].y>= play_y ) { 
                status= SIV_STATUS_DEATH;
                break; 
            }
        }
    }
    // check du tir des monstre
        // checker remainaing monsters
    for (int i=0;i<array_max;i++) {
       // printf("deplace_enemy::monster_tir %p index[%d] \n", &m_monst_tir[i], i);
  
        if (m_monst_tir[i].monster ==NULL) {
            m_monst_tir[i].monster = choose_monster_fire(&m_monst_tir[i]);
           // printf("deplace_enemy::monster chosen %p for enemy %d\n", m_monst_tir[i].monster, i);
        }
        
    }
    // affiche tir + detection fin
    for (int i=0;i<array_max;i++) {
        if (m_monst_tir[i].monster !=NULL) {
            rct.x= m_monst_tir[i].x;
            rct.y= m_monst_tir[i].y;
            rct.w=75/SCREEN_RATIO;
            rct.h=115/SCREEN_RATIO;
            if (m_monst_tir[i].monster->type !=1)
                m_monst_tir[i].y+=4*dif_level;
            else m_monst_tir[i].y+=8*dif_level;

            if (is_tir_bottom(&m_monst_tir[i])) { 
                //affiche tir vide
                SDL_RenderCopy(pRenderer, pSprite, &spr_empty[0], &rct);
            } else {
                SDL_RenderCopy(pRenderer, pSprite, &spr_tir_monster[0], &rct);
            } 
            if (m_monst_tir[i].x+46/SCREEN_RATIO >= play_x && m_monst_tir[i].x+46/SCREEN_RATIO <= play_x+75/SCREEN_RATIO && m_monst_tir[i].y>=play_y ) {
//                printf("DEPLACE_ENEMY:IN\n");
                status = SIV_STATUS_DEATH;
               // printf("DEPLACE_ENEMY:firing  %p\n", m_monst_tir[i].monster);
                //m_monst_tir[i].monster->firing = 0;
 
          //      m_monst_tir[i].monster = NULL;
           //     printf("DEPLACE_ENEMY:OUT\n");
                break;
            }

        }
    }
   display_enemy();
}
int Create() {
    if (SDL_Init(SDL_INIT_VIDEO) !=0) {
        printf("Erreur d'initialisation de la SDL : %s\n", SDL_GetError() );
        return -1;
    }

    pWindow = SDL_CreateWindow("SPACE INVUDERS", 
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                SIV_WND_WIDTH, SIV_WND_HEIGHT,SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI );
    //SDL_SetWindowFullscreen(pWindow, SDL_WINDOW_FULLSCREEN);
                            
    if (pWindow) {
            if (!TTF_Init()) {
                if ( !(pFont = TTF_OpenFont("freefont/FreeMono.ttf", 16)) ) {
                    printf("Erreur lors du chargement de la font %s \n", SDL_GetError());
                    return -1;
                } else {
                    if ((pRenderer = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) )==NULL) {
                        printf("Erreur lors de la création du Renderer %s \n", SDL_GetError());
                        return -1;
                    } else {
                        sprite_sfc = IMG_Load("sprites2.bmp");
                        SDL_SetColorKey(sprite_sfc, SDL_TRUE, SDL_MapRGB(sprite_sfc->format, 0x00, 0x00, 0x00));
                        pSprite = SDL_CreateTextureFromSurface(pRenderer, sprite_sfc);
                        SDL_ShowCursor(SDL_DISABLE );

                        return 1;
                    }
                }
            } else {
                printf("Erreur lors de la création de TTF_Init %s \n", SDL_GetError());
                return -1;

            }
    } else {
        printf("Erreur lors de la création de la fenêtre %s \n", SDL_GetError());
        return -1;
    }
}

void Destroy() {
    SDL_FreeSurface(sprite_sfc);
    SDL_DestroyTexture(pSprite);
    SDL_DestroyTexture(pScene);
    TTF_CloseFont(pFont);
    SDL_DestroyWindow(pWindow);
    SDL_Quit();
}

void Display_Text(const char* pszTitle, int attrb, SDL_Color *pColor, int x, int y, int filled) {
    SDL_Surface* pSurface = TTF_RenderText_Solid(pFont, pszTitle, *pColor );
    SDL_Texture* pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
    SDL_FreeSurface(pSurface);
    int texW, texH;
    SDL_QueryTexture(pTexture, NULL, NULL, &texW, &texH);
    SDL_Rect dstrect = {0,0, texW, texH};
    switch(attrb) {
        case SIV_TXT_CENTER:
            dstrect.x = (SIV_WND_WIDTH - texW) /2;
            dstrect.y = y;
        break;
        case SIV_TXT_LEFT:
            dstrect.x = x;
            dstrect.y = y;
        break;
        default:
        break;
    }
    if (filled == 1) {
        SDL_SetRenderDrawColor( pRenderer, 0, 0, 0, 0 );
        SDL_RenderFillRect( pRenderer, &dstrect );
    }
    SDL_RenderCopy(pRenderer, pTexture, NULL, &dstrect );
    SDL_DestroyTexture(pTexture);
}

void Update_GameOver() {
        SDL_Color color = {255,0,0};
        SDL_SetRenderDrawColor(pRenderer, 0,0,0, 255);
        SDL_RenderClear(pRenderer);
        Display_Text("Fin du Jeu", SIV_TXT_CENTER, &color, 0,96,0);
        char s_params[255];
        sprintf(s_params,"Score :%d", score);
        Display_Text(s_params, SIV_TXT_CENTER, &color, 0,104,0);
        if (score>high_score) {
            high_score = score;
            Display_Text("Nouveau High Score", SIV_TXT_CENTER, &color, 0,112,0);
        }
        vie =0;
        score =0;
        SDL_RenderPresent(pRenderer);
        SDL_Delay(2500);
        initialize_level();
        sfc_loaded = 0;
        dif_level=1;
        vie =3;
        status = SIV_GAME;

}
// Choose_level:
// Ne sert à rien dans le contexte

void choose_level() {
}

void Update_NextLevel() {
        SDL_Delay(2500);
        dif_level ++;
        initialize_level();
        choose_level();
        score = score +10000;  
        status = SIV_GAME;
}



void Init_Scene() {
    SDL_Surface* scene_sfc = SDL_CreateRGBSurface(0,SIV_WND_WIDTH,SIV_WND_HEIGHT,32,0,0,0,0);

    if (pScene !=NULL) SDL_DestroyTexture(pScene);
        pScene = SDL_CreateTextureFromSurface(pRenderer, scene_sfc);
    SDL_FreeSurface(scene_sfc);


}
// Return true if still jumping


void Move_Vessel() {
                if (a_ks[KS_RIGHT].status == KS_KEYDOWN) {
                    if ( (play_x+=16/SCREEN_RATIO)>=934 )
                        play_x = 934;
                }
                if (a_ks[KS_LEFT].status == KS_KEYDOWN) {
                    if ( (play_x-=16/SCREEN_RATIO)<=0)
                        play_x = 0;
                }

}

// Return 1 if collided 0 if not
int _is_monster_collided (Monster *m) {
    int res =0;
            if (m->active==1) {
                if ((tir_x +(45/SCREEN_RATIO) > m->x ) && (tir_x+(45/SCREEN_RATIO) < m->x+(75/SCREEN_RATIO) ) && (tir_y < m->y+(115/SCREEN_RATIO) ) && (tir_y > m->y) ) {
                    m->active = 0;
                    m_remaining_monsters--;
                    m->status = 2;
                    res = 1;
                }
                    
            }
         //   printf("Remaining monster %d\n",m_remaining_monsters );
    return res;

}
int is_collided() {
    int a=0;
    for (int i=0;i<MAX_MONSTRE;i++) {
        a=a+_is_monster_collided(&m_sirena[i]);
        a=a+_is_monster_collided(&m_sconz[i]);
        a=a+_is_monster_collided(&m_flappy[i]);
        a=a+_is_monster_collided(&m_feliz[i]);
    }
    // missile colides bunker
    // missile colides vessel
    // missile colides alien

    return (a==0 ? 0 : 1);
}

void Update_Game() {
 
   
        // Affichage des paramètres:

    SDL_Delay(30); // 
    if (sfc_loaded==0) {
            Init_Scene(); // si on change de niveau et que la scene doit être recréé via une nouvelle surface.
            initialize_level(); // sprites coordinates;
            sfc_loaded=1;
    }

        // Create Screen

    SDL_Rect rct_scene = {0,0,SIV_WND_WIDTH,SIV_WND_HEIGHT};
    SDL_RenderCopy(pRenderer, pScene, &rct_scene, &rct_scene );
    SDL_Rect rct;
  
        // Collision
 
        Move_Vessel();
        deplace_enemy();

        if (tir_status) {
            if (is_collided() ) {
                tir_status = 0;
                rct.x= tir_x;
                rct.y= tir_y;
                rct.w=75/SCREEN_RATIO;
                rct.h=115/SCREEN_RATIO;
                score +=100;
                SDL_RenderCopy(pRenderer, pSprite, &spr_empty[0], &rct);

            } else if (tir_y<=0) {
                tir_status =0;
                rct.x= tir_x;
                rct.y= tir_y;
                rct.w=75/SCREEN_RATIO;
                rct.h=115/SCREEN_RATIO;
                SDL_RenderCopy(pRenderer, pSprite, &spr_empty[0], &rct);

            }
            tir_y =tir_y-8;
            rct.x= tir_x;
            rct.y= tir_y;
            rct.w=75/SCREEN_RATIO;
            rct.h=115/SCREEN_RATIO;
            SDL_RenderCopy(pRenderer, pSprite, &spr_tir[0], &rct);
        }
    
        spr_player_dest.x = play_x;
        spr_player_dest.y = play_y;
        SDL_RenderCopy(pRenderer, pSprite, &spr_player[0], &spr_player_dest);
    
    
        
    
    SDL_SetRenderDrawColor(pRenderer, 0,0,0, 255);
    SDL_Color color = {255,255,255};
    char s_params[255];
    sprintf(s_params, "Score=%d        HIGH Score= %d     Vie(s)=%d", score, high_score, vie);
    Display_Text(s_params, SIV_TXT_CENTER , &color, 0,752,0);
    

}

void update_death() {
        // Create Screen
    SDL_Rect rct_scene = {0,0,SIV_WND_WIDTH,SIV_WND_HEIGHT};
    SDL_RenderCopy(pRenderer, pScene, &rct_scene, &rct_scene );
    //SDL_Rect rct;
    deplace_enemy();
  /*  for (int i=0;i<max_enemy;i++)
       display_enemy(i);*/

// display-empty
    SDL_RenderCopy(pRenderer, pSprite, &spr_empty[0], &spr_player_dest);

// display - fire one
    SDL_RenderCopy(pRenderer, pSprite, &spr_expl_enemy[1], &spr_player_dest);
    SDL_RenderPresent(pRenderer);

    SDL_Delay (350);
// display - fire to
    SDL_RenderCopy(pRenderer, pSprite, &spr_expl_enemy[2], &spr_player_dest);
    SDL_RenderPresent(pRenderer);

    SDL_Delay (350);
    SDL_RenderCopy(pRenderer, pSprite, &spr_empty[0], &spr_player_dest);
    SDL_Delay(2500);
    vie --;
    if (vie>0) {

        status = SIV_GAME;
    } else 
        status = SIV_GAME_OVER;

}
void update_status() {
    SDL_SetRenderDrawColor(pRenderer, 0,0,0, 255);
    SDL_RenderClear(pRenderer);

    switch(status) {
        case SIV_GAME:
                Update_Game();
        break;
        case SIV_STATUS_DEATH:
                update_death();
        break;
        case SIV_GAME_OVER:
            Update_GameOver();
        break;
    
        case SIV_NEXT:
            Update_NextLevel();
        break;
        }
    SDL_RenderPresent(pRenderer);
}

int main(int argc, char *argv[]) {
    // Intialize le générateur congruentiel par rapport à l'horloge 
    time_t t;
    t = time(NULL);
    srand(t);
    
    // Initialisation de la première scene du jeu
    // ------------------------------------------
    last_time = SDL_GetTicks();

    SDL_Event event;
  
    Initialize_ks();
    if (Create()==1) {
        while (q==0) {
            
         
            update_status();
              //  Process_Game();
            while (SDL_PollEvent(&event) != 0) {
                switch (event.type) {
                    case SDL_QUIT:
                        q=1;
                    break;
                    case SDL_KEYUP:
                        if (event.key.keysym.sym==SDLK_ESCAPE) q=1;                    
                        if (event.key.keysym.sym==SDLK_RIGHT) {
                            a_ks[KS_RIGHT].status = KS_KEYUP;
                        } else if (event.key.keysym.sym==SDLK_LEFT) {
                            a_ks[KS_LEFT].status = KS_KEYUP;
                        } else if (event.key.keysym.sym==SDLK_UP) {
                            a_ks[KS_UP].status = KS_KEYUP;
                        }  else if (event.key.keysym.sym==SDLK_SPACE){
                            a_ks[KS_SPACE].status = KS_KEYUP;
                        }
                        

                    break;
                    case SDL_KEYDOWN:
                        if (event.key.keysym.sym==SDLK_RIGHT) {
                            a_ks[KS_RIGHT].status = KS_KEYDOWN;
                        } else if (event.key.keysym.sym==SDLK_LEFT) {
                            a_ks[KS_LEFT].status = KS_KEYDOWN;
                        } else if (event.key.keysym.sym==SDLK_UP) {
                            a_ks[KS_UP].status = KS_KEYDOWN;
                        } else if (event.key.keysym.sym==SDLK_SPACE){
                            if (tir_status==0) {
                                tir_status = 1;
                                tir_x = play_x;
                                tir_y = play_y-115/SCREEN_RATIO;
                            }
                        }
                        
                    break;
                    default:
                    break;
                }
            }
        }

    }

    Destroy();
    exit(0);
    return 0;
}


