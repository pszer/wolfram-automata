#include <SDL2/SDL.h>

#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define WINPOS SDL_WINDOWPOS_UNDEFINED

SDL_Window   * win;
SDL_Renderer * render;
SDL_Event      event;

int WIDTH  = 512;
int HEIGHT = 512;
int do_random = 1;
int seed;

char * state;
char * next;

// e6df9a
//                r    g    b    a
SDL_Color C1 = { 0xE6,0xDF,0x9A,0xff };
SDL_Color C0 = { 0x00,0x00,0x00,0xff };

void start_state(char * s);
void random_state(char * s);
void single_state(char * s);

char is_integer(const char * str);
void draw_state(char * s, int y);
void set_color(SDL_Color * c);

char get_state(char * s, int x);
void calc_new_state(char * curr, char * next);

void gen_rules(int rule);
char get_rule(char left, char middle, char right);

void draw_rule(int rule, char * state, char * next, int seed);

char rules[8];

#define ERROR(func) {func;return -1;}
int main(int argc, char ** argv) {
	int rule;
	seed = time(NULL);

	if (argc < 2)
		ERROR(puts("rule [rule 0-255] {seed} {w} {h} {single}\n"
		           "default seed is time()\n"
		           "default w h are 512 512\n"
		           "default start state is random"));

	#define ISINT(n) argc>n && !is_integer(argv[n])
	if (ISINT(1))
		ERROR(puts("rule given should be an integer 0-255"));
	if (ISINT(2))
		ERROR(puts("seed should be an integer 0-255"));
	if (ISINT(3))
		ERROR(puts("width should be an integer"));
	if (ISINT(4))
		ERROR(puts("height should be an integer"));
	if (argc > 5 && strcmp(argv[5],"single"))
		ERROR(puts("unknown 5th argument"));

	char * endptr;
	#define GETNUM(a,n) if(argc>n)a=strtol(argv[n],&endptr,0);
	GETNUM(rule,1);
	GETNUM(seed,2);
	GETNUM(WIDTH,3);
	GETNUM(HEIGHT,4);
	if (argc > 5) do_random = 0;

	if (rule > 255)
		ERROR(puts("rule out of range"));

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		ERROR(printf("SDL: %s\n",SDL_GetError()));

	win = SDL_CreateWindow(argv[1], WINPOS, WINPOS,
		WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!win) ERROR(printf("SDL: %s\n",SDL_GetError()));

	render = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
	if (!render) ERROR(printf("SDL: %s\n",SDL_GetError()));

	state = malloc(sizeof(char) * WIDTH);
	next  = malloc(sizeof(char) * WIDTH);

	draw_rule(rule, state, next, seed);

	int going=1;
	while (going) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_KEYDOWN) {
				switch (event.key.keysym.sym) {
				case SDLK_UP:
					++rule;
					rule %= 0x100;
					break;
				case SDLK_DOWN:
					--rule;
					if (rule < 0) rule = 0xff;
					break;
				case SDLK_SPACE:
					do_random = !do_random;
					break;
				case SDLK_r:
					seed = rand();
				}
				draw_rule(rule, state, next, seed);
			} else if (event.type == SDL_QUIT) {
				going = 0;
			}
		}

		SDL_RenderPresent(render);
		SDL_Delay(15);
	}

	free(state);
	free(next);

	SDL_Quit();
	return 0;
}

char is_integer(const char * str) {
	const char * c;
	for (c = str; *c; ++c)
		if (!isdigit(*c)) break;
	return !*c;
}

void start_state(char * s) {
	if (do_random) random_state(s);
	else           single_state(s);
}

void random_state(char * s) {
	int i;
	srand( seed );
	for (i = 0; i < WIDTH; ++i)
		s[i] = rand() % 2;
}

void single_state(char * s) {
	int i, mid = WIDTH/2;
	for (i = 0; i < WIDTH; ++i) {
		if (i == mid)
			 s[i] = 1;
		else s[i] = 0;
	}
}

void draw_state(char * s, int y) {
	int x;
	for (x = 0; x < WIDTH; ++x) {
		SDL_Color * C = s[x] ? &C1 : &C0;
		set_color(C);
		SDL_RenderDrawPoint(render, x, y);
	}
}

void set_color(SDL_Color * c) {
	SDL_SetRenderDrawColor(render, c->r, c->g, c->b, c->a);
}

char get_state(char * s, int x) {
	if (x < 0 || x >= WIDTH)
		return 0;
	return s[x];
}

void gen_rules(int rule) {
	printf("rule %i\n", rule);
	const char masks[8] =
		{0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x1};
	int i;
	for (i = 0; i < 8; ++i) {
		int j = 7-i;
		rules[i] = rule&masks[i] ? 1:0;

		char l = (j & masks[8-3])/4 + '0';
		char m = (j & masks[8-2])/2 + '0';
		char r = (j & masks[8-1])/1 + '0';

		printf("%c%c%c ", l, m, r);
	}
	putchar('\n');

	for (i = 0; i < 8; ++i) {
		printf(" %c  ", rules[i]+'0');
	}
	putchar('\n');
}

char get_rule(char left, char middle, char right) {
	return 0x7 -
	       (left   * 4) -
	       (middle * 2) -
	       (right  * 1);
}

void calc_new_state(char * curr, char * next) {
	int i;
	for (i = 0; i < WIDTH; ++i) {
		char l = get_state(curr, i-1);
		char m = get_state(curr, i+0);
		char r = get_state(curr, i+1);
		int rule_got = get_rule(l,m,r);

		char new_state = rules[ rule_got ];
		next[i] = new_state;
	}
}

void draw_rule(int rule, char * state, char * next, int seed) {
	start_state(state);
	gen_rules(rule);

	SDL_RenderClear(render);

	#define SWAP(a,b) char*temp=a;a=b;b=temp
	int y;
	for (y = 0; y < HEIGHT; ++y) {
		draw_state(state, y);
		calc_new_state(state, next);
		// swap buffers without having
		// to move any memory
		SWAP(state, next);
	}
}
