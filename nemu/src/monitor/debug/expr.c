#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, 

  /* TODO: Add more token types */
  TK_NUMBER,TK_HEX,TK_REG,
  TK_EQ,TK_NEQ,//TK_AND,TK_OR,
  TK_NEGATIVE,TK_DEREF

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\$(eax|ecx|edx|ebx|esp|ebp|esi|edi|eip|ax|cx|dx|bx|sp|bp|si|di|al|cl|dl|bl|ah|ch|dh|bh)", TK_REG}, 
  {"0x[0-9a-fA-F]+", TK_HEX},     // hexadecimal integer
  {"0|[1-9][0-9]*",TK_NUMBER},

  {"==", TK_EQ},                     // equal
  {"!=", TK_NEQ}, 

  {"\\+", '+'},                  // plus
  {"-", '-'},                 // minus
  {"\\*", '*'},                 // times
  {"\\/", '/'},                   // division               // not
  
  {"\\(", '('},                // left parenthesis
  {"\\)", ')'}                // right parenthesis
};


#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') 
{
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) 
    {
      if (regexec(&re[i],e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) 
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

	if(substr_len>32)
	    assert(0);
        if(rules[i].token_type==TK_NOTYPE) 
	    break;
	else
	{
	    tokens[nr_token].type = rules[i].token_type;
	    switch(rules[i].token_type)
	{case TK_NUMBER: strncpy(tokens[nr_token].str,substr_start,substr_len);
	   *(tokens[nr_token].str+substr_len)='\0';break;
          //default: TODO();
        }
	nr_token+=1;
	printf("some information of the token: type=%d,token=%s\n",tokens[nr_token].type,tokens[nr_token].str);
        break;
	}
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static int check_parentheses(int p, int q) 
{
	if(p>=q){printf("error:p>=q in check\n");return 0;}
	if(tokens[p].type!='('||tokens[q].type!=')') return 0;

	int count=0;
	for(int curr=p+1;curr<q;curr++)
	{
	  if(tokens[curr].type=='(') count++;
	  if(tokens[curr].type==')') 
	  {
		if(count!=0) count--;
		else return 0;
	  }
	}
	if(count==0) return 1;
	else return 0;
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  //TODO();
  if(tokens[0].type=='-')
	tokens[0].type=TK_NEGATIVE;
  if(tokens[0].type=='*')
	tokens[0].type=TK_DEREF;
  for(int i=1;i<nr_token;i++)
  {
    if(tokens[i].type=='-'&&tokens[i-1].type!=TK_NUMBER && tokens[i-1].type!=')')
	tokens[i].type=TK_NEGATIVE;
    if(tokens[i].type=='*'&&tokens[i-1].type!=TK_NUMBER && tokens[i-1].type!=')')
	tokens[i].type=TK_DEREF;
  }
  *success = true;
  return 0;
}
