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
  TK_EQ,TK_NEQ,TK_AND,TK_OR,
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

  {"&&",TK_AND},
  {"\\|\\|",TK_OR},
  {"!",'!'},

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

        /* Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);*/
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

	if (rules[i].token_type != TK_NOTYPE) { // ignore spaces
          assert(substr_len >= 0 && substr_len < 32);
          tokens[nr_token].type = rules[i].token_type;
          memset(tokens[nr_token].str, 0, sizeof(tokens[nr_token].str));
          strncpy(tokens[nr_token].str, substr_start, substr_len);
          tokens[nr_token].str[substr_len] = '\0';
	  printf("some information of the token: type=%d,token=%s\n",tokens[nr_token].type,tokens[nr_token].str);
          nr_token++;
        }
        break;
	
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}
/*
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
}*/

static int check_parentheses(int p, int q) {
	assert(p <= q);
	int estack[32];
	memset(estack, -1, sizeof(estack));
	int i, etop = -1;
	int lastmatchlp = -1, lastmatchrp = -1;
	for (i = p; i <= q; i++) 
	{
		if (tokens[i].type == '(') estack[++etop] = i;
		if (tokens[i].type == ')') 
		{
			if (etop == -1)  return -2; 
			lastmatchlp = estack[etop--];
			lastmatchrp = i;
		}
	}
	if (etop != -1)  return -2; 
	if (lastmatchlp != p || lastmatchrp != q) 
	{
		if (tokens[p].type == '(' && tokens[q].type == ')')  return -3; 
		return -1; 
	}
	return 1;
}

static bool is_op(int i) 
{
	if (i != TK_NUMBER && i != TK_HEX && i != TK_REG && i != TK_NOTYPE) return true;
	return false;
}

static int priority(int token_type) 
{
	switch (token_type) {
	  case TK_OR: return 1;
	  case TK_AND: return 2;
	  case TK_EQ:
	  case TK_NEQ: return 3;
	  case '+':
	  case '-': return 4;
	  case '*':
	  case '/': return 5;
	  case TK_NEGATIVE:
	  case TK_DEREF:
	  case '!': return 6;
	  default: return -1;
	}
}

static int judge_prior(int sstack, int curr) 
{
	if (priority(sstack) == -1 || priority(curr) == -1) return 0;
	if (priority(sstack) > priority(curr))  return 1;
	else if (priority(sstack) < priority(curr)) return -1;
	else 
	{
		int pstack = priority(sstack);
		if (pstack != 6)  return 1;
		else   return -1;
	}
}

static int dominant_operator(int p, int q) 
{
	assert(p < q);
	int estack[32];
	memset(estack, -1, sizeof(estack));
	int i, etop = -1;
	for (i = p; i <= q; i++)
	{
		if (!is_op(tokens[i].type)) continue;
		if (etop == -1) estack[++etop] = i;
		else if (tokens[estack[etop]].type == '(') estack[++etop] = i;
		else if (tokens[i].type == '(') estack[++etop] = i;
		else if (tokens[i].type == ')') 
		{
			while (etop != -1 && tokens[estack[etop]].type != '(') etop--;
			if (etop != -1 && tokens[estack[etop]].type == ')') etop--;
		}
		else if (judge_prior(tokens[estack[etop]].type, tokens[i].type) < 0)  estack[++etop] = i;
		else if (judge_prior(tokens[estack[etop]].type, tokens[i].type) > 0) 
		{
			while (etop != -1 && judge_prior(tokens[estack[etop]].type, tokens[i].type) > 0)  etop--;
			estack[++etop] = i;
		}
	}
	if (etop < 0) return -1;
	return estack[0];
}

static uint32_t eval(int p, int q, bool *success) 
{
	if (p > q) {*success = false;return -1;}
	if (p == q) 
	{
		uint32_t n = -1;*success = true;
		if (tokens[p].type == TK_NUMBER) {sscanf(tokens[p].str, "%u", &n);return n;}
		else if (tokens[p].type == TK_HEX) {sscanf(tokens[p].str, "%x", &n);return n;}
		else if (tokens[p].type == TK_REG) 
		{
			uint32_t i;
			char *str = tokens[p].str + 1;
    			for (i = 0; i < 8; i++) 
			{if (strcmp(reg_name(i, 4), str) == 0) {n = reg_l(i);return n;}}
    			if (strcmp("eip", str) == 0) {n = cpu.eip;return n;}
    			for (i = 0; i < 8; i++) if (strcmp(reg_name(i, 2), str) == 0) {n = reg_w(i);return n;}
    			for (i = 0; i < 8; i++) if (strcmp(reg_name(i, 1), str) == 0) {n = reg_b(i);return n;}
		}
		*success = false;
		return n;
	}
	else if (check_parentheses(p, q) == 1)  return eval(p + 1, q - 1, success);
	else 
	{
		int op = dominant_operator(p, q);
		if (op < 0) {*success = false;return -1;}
		uint32_t val = eval(op + 1, q, success);
		if (*success == false) 		return -1;
		switch (tokens[op].type) 
		{
		  case TK_NEGATIVE: return -val;
		  case TK_DEREF: return vaddr_read(val, 4);
		  case '!': return !val;
		}
		uint32_t val1 = eval(p, op - 1, success);
		if (*success == false) return -1;
		uint32_t val2 = eval(op + 1, q, success);
		if (*success == false) return -1;
		switch (tokens[op].type) 
		{
		  case '+': return val1 + val2;
		  case '-': return val1 - val2;
		  case '*': return val1 * val2;
		  case '/': 
		    if (val2 == 0) {printf("Error: Divide by 0 !\n");*success = false;return -1;}
		    assert(val2 != 0);
		    return val1 / val2;
		  case TK_AND: return val1 && val2;
		  case TK_OR: return val1 || val2;
		  case TK_EQ: return val1 == val2;
		  case TK_NEQ: return val1 != val2;
		  default: assert(0);
		}
	}
}

uint32_t expr(char *e, bool *success) 
{
  if (!make_token(e)) 
  {
    *success = false;
    return 0;
  }
  if (check_parentheses(0, nr_token - 1) == -2) 
  {
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
  //return 0;
  return eval(0,nr_token-1,success);
}
