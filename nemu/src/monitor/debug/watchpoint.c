#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

void print_wp()
{
  if(head==NULL){printf("no watchpoint now\n");return n;}
  printf("watchpoint:\n");
  printf("NO.    EXPR        hitTimes\n");
  wptemp=head;
  while(wptemp!=NULL)
  {
    printf("%d    %s        %d\n",wptemp->NO,wptemp->e,wptemp->hitNum);
    wptemp=wptemp->next;
  }
}



