#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */

  { "si", "Step [N] instructions exactly (1 step if no args given), usage: si [N]", cmd_si },
  { "info", "Display informations about registers and watchpoints in the program being debugged, usage: info r / info w", cmd_info },
  { "x", "Examine memory, usage: x [N] [EXPR]", cmd_x },
  { "p", "Print value of expression, usage: p [EXPR]", cmd_p },
  { "w", "Set a watchpoint for an expression, usage: w [EXPR]", cmd_w },
  { "d", "Delete watchpoint number [N] (delete all if no args given), usage: d [N]", cmd_d }

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

//my code
static int cmd_si(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  uint64_t n = 0;

  if (arg == NULL) {
    /* no argument given */
    cpu_exec(1); // 1 is the default
  }
  else {
    char *temp = strtok(NULL, " ");
    if (temp == NULL) {
      sscanf(arg, "%llu", &n);
      cpu_exec(n); // Execute n times
    }
    else {
      printf("Syntax error: Too much arguments.\n");
    }
  }
  return 0;
}

static int cmd_info(char *args) 
{
  char s;
  if(args==NULL){ printf("no args in cmd_info!\n");  return 0;}

  int nRet=sscanf(args,"%c",&s);
  if(nRet<=0) {printf("args error in cmd_info\n");  return 0;}
  if(s=='r')
  {
     int i;
     for(i=0;i<8;i++) printf("%s    0x%x\n",regsl[i],reg_l(i));
     printf("eip    0x%x\n",cpu.eip);
     for(i=0;i<8;i++) printf("%s    0x%x\n",regsw[i],reg_w(i));
     for(i=0;i<8;i++) printf("%s    0x%x\n",regsb[i],reg_b(i));
     return 0;
  }
  else if(s=='w')
  {print_wp();return 0;}
  return 0;
}


static int cmd_x(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  uint32_t n = 0;

  if (arg == NULL) {
    /* no argument given */
    printf("Argument required (starting display address).\n");
  }
  else {
    char *temp1 = strtok(NULL, " ");
    char *temp2 = strtok(NULL, " ");
    uint32_t start_addr = 0;
    uint32_t mem_data = 0;
    bool success = false;
    if (temp2 == NULL) {
      sscanf(arg, "%u", &n);
      start_addr = expr(temp1, &success);
      if (!success) {
      	printf("Illegal expression.\n");
      	return 0;
      }
      uint32_t i, j;
      for (i = 0; i < n ; i++) {
      	printf("0x%x:    ", start_addr);
      	for (j = 0; j < 4; j++) {
      		mem_data = vaddr_read(start_addr, 1);
      		printf("0x%02x    ", mem_data);
      		start_addr++;
      	}
      	printf("\n");
      }
    }
    else {
      printf("Syntax error: Too much arguments.\n");
    }
  }
  return 0;
}

static int cmd_p(char *args) 
{
  if (args == NULL) 
  {
    printf("Argument required (expression to compute).\n");
    return 0;
  }
  bool success = false;
  int result = expr(args, &success);
  if (success) 
  {
	printf("expr_value = %d\n", result);
  }
  else printf("Illegal expression.\n");
  return 0;
}

static int cmd_w(char *args) 
{
  if (args == NULL) 
  {
    printf("Argument required (expression to compute).\n");
    return 0;
  }
  new_wp(args);
  return 0;
}

static int cmd_d(char *args) {
  int num=0;
  int nRet=sscanf(args,"%d",&num);
  if(nRet<=0){printf("args err in cmd_si\n");return 0;}
  int r=free_wp(num);

  if(r==0) printf("err: no wp %d\n",num);
  else  printf("success delete wp %d\n",num);

  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }


#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
