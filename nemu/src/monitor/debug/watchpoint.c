#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_,*wptemp;
static int used_next;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
    wp_pool[i].oldValue=0;
    wp_pool[i].hitNum=0;
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
  used_next=0;
}

/* TODO: Implement the functionality of watchpoint */

bool new_wp(char *args) 
{
	if (free_ == NULL) assert(0);
	WP *temp = free_;
	free_ = free_->next;
	
	temp->hitNum = 0;

	temp->NO = used_next;used_next++;
	temp->next = NULL;
	strcpy(temp->e, args);
	bool success;
	temp->oldValue=expr(temp->e,&success);
	if(success==false)
	{printf("err in new_wp:invalid expr!!\n");return 0;}

	wptemp = head;
	if (wptemp == NULL) head = temp;
	else 
	{
		while (wptemp->next != NULL) wptemp = wptemp->next;
		wptemp->next = temp;
	}
	printf("success:set wp %d,oldValue=%d\n",temp->NO,temp->oldValue);
	return 1;
}

void print_wp()
{
  if(head==NULL){printf("no watchpoint now\n");return;}
  printf("watchpoint:\n");
  printf("NO.    EXPR        hitTimes\n");
  wptemp=head;
  while(wptemp!=NULL)
  {
    printf("%d    %s        %d\n",wptemp->NO,wptemp->e,wptemp->hitNum);
    wptemp=wptemp->next;
  }
}



