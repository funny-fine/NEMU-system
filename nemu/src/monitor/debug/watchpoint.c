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

bool free_wp(int num) 
{
	if (head == NULL) {printf("no wp now!\n");return 0;}
	WP *find = NULL;
	if (head->NO == num) 
	{
		find = head;
		head = head->next;
	}
	else 
	{
		wptemp = head;
		while (wptemp!=NULL && wptemp->next != NULL) 
		{
			if (wptemp->next->NO == num) {
				find = wptemp->next;
				wptemp->next = find->next;
				break;
			}
			wptemp = wptemp->next;
		}
	}
	if (find == NULL) return 0;
	find->next = free_;
	free_ = find;
	return true;
}

void print_wp()
{
  if(head==NULL){printf("no watchpoint now\n");return;}
  printf("watchpoint:\n");
  printf("%-5s%-15s%s\n", "Num", "Value", "Hit count");
  //printf("%-5s%-15s%s\n\n","NO.","EXPR","hitTimes");
  wptemp=head;
  while(wptemp!=NULL)
  {
    printf("%-5d%-15s%d\n\n",wptemp->NO,wptemp->e,wptemp->hitNum);
    wptemp=wptemp->next;
  }
}

bool watch_wp() 
{
	int val;
	bool success = 0;
	if(head==NULL) return 1;
	wptemp = head;
	while (wptemp != NULL) 
	{
		val = expr(wptemp->e, &success);
		if (val!=wptemp->oldValue) 
		{
			wptemp->hitNum++;
			printf("Watchpoint %d: %s\n\n", wptemp->NO, wptemp->e);
			printf("Old value = %d\n", wptemp->oldValue);
			printf("New value = %d\n", val);
			wptemp->oldValue=val;
			return 0;
		}
		wptemp = wptemp->next;
	}
	return 1;
}



