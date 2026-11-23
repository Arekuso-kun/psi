#include "calculator.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const char *p; McError error; } Parser;
static int digit(char ch) { return ch >= '0' && ch <= '9'; }
static int op(char ch) { return ch=='+' || ch=='-' || ch=='*' || ch=='/'; }
static void spaces(Parser *p) { while (*p->p==' ' || *p->p=='\t') ++p->p; }
static double number(Parser *p) {
    const char *start; char token[MC_EXPRESSION_MAX+1];
    size_t length; int digits=0; double value;
    spaces(p); start=p->p;
    if (*p->p=='+' || *p->p=='-') ++p->p;
    while (digit(*p->p)) { ++p->p; ++digits; }
    if (*p->p=='.') { ++p->p; while (digit(*p->p)) { ++p->p; ++digits; } }
    if (!digits) { p->error=MC_SYNTAX; return 0; }
    if (*p->p=='e' || *p->p=='E') {
        int exponent_digits=0;
        ++p->p; if (*p->p=='+' || *p->p=='-') ++p->p;
        while (digit(*p->p)) { ++p->p; ++exponent_digits; }
        if (!exponent_digits) { p->error=MC_SYNTAX; return 0; }
    }
    length=(size_t)(p->p-start);
    if (length>MC_EXPRESSION_MAX) { p->error=MC_FULL; return 0; }
    memcpy(token,start,length);token[length]='\0';
    errno=0;value=strtod(token,0);
    if (errno==ERANGE || !isfinite(value)) p->error=MC_RANGE;
    return value;
}
static double term(Parser *p) {
    double left=number(p);
    while (!p->error) {
        char operation; double right;
        spaces(p); operation=*p->p;
        if (operation!='*' && operation!='/') break;
        ++p->p;right=number(p);if (p->error) break;
        if (operation=='/' && right==0.0) { p->error=MC_DIV_ZERO; break; }
        left=operation=='*' ? left*right : left/right;
        if (!isfinite(left)) p->error=MC_RANGE;
    }
    return left;
}
McError mc_evaluate(const char *expression,double *result) {
    Parser p; double left;
    if (!expression || !result) return MC_SYNTAX;
    if (strlen(expression)>MC_EXPRESSION_MAX) return MC_FULL;
    p.p=expression;p.error=MC_OK;left=term(&p);
    while (!p.error) {
        char operation;double right;
        spaces(&p);operation=*p.p;
        if (!operation) break;
        if (operation!='+' && operation!='-') { p.error=MC_SYNTAX; break; }
        ++p.p;right=term(&p);if(p.error) break;
        left=operation=='+' ? left+right : left-right;
        if (!isfinite(left)) p.error=MC_RANGE;
    }
    if (!p.error) *result=left==0.0 ? 0.0 : left;
    return p.error;
}
void mc_calculator_init(McCalculator *c) { memset(c,0,sizeof(*c));strcpy(c->result,"0"); }
static int append(McCalculator *c,const char *text) {
    size_t n=strlen(c->expression),m=strlen(text);
    if (n+m>MC_EXPRESSION_MAX) { c->error=MC_FULL;return 0; }
    memcpy(c->expression+n,text,m+1);return 1;
}
static size_t operand(const char *s) {
    size_t i,start=0;
    for(i=0;s[i];++i) if(op(s[i])) {
        if(i && (s[i-1]=='e'||s[i-1]=='E')) continue;
        if(s[i]=='-' && (!i || op(s[i-1]))) continue;
        start=i+1;
    }
    return start;
}
static void display_result(McCalculator *c) {
    if (c->value==0.0) c->value=0.0;
    (void)snprintf(c->result,sizeof(c->result),"%.10g",c->value);
}
static void continue_result(McCalculator *c) {
    (void)snprintf(c->expression,sizeof(c->expression),"%.15g",c->value);
    c->evaluated=0;
}
int mc_calculator_key(McCalculator *c,McKey key) {
    size_t n,start; McError error;
    if (key>MC_SIGN) return 0;
    if(key==MC_CLEAR) { mc_calculator_init(c);return 1; }
    if(key==MC_DELETE) {
        if(c->evaluated) { mc_calculator_init(c);return 1; }
        n=strlen(c->expression);if(!n) return 0;
        c->expression[n-1]='\0';c->error=MC_OK;return 1;
    }
    c->error=MC_OK;
    if(key==MC_EQUAL) {
        double result;
        error=mc_evaluate(c->expression,&result);
        if(error) { c->error=error;return 1; }
        c->value=result;c->evaluated=1;display_result(c);return 1;
    }
    if(key<=MC_9 || key==MC_DOT) {
        char text[3]={0};
        if(c->evaluated) mc_calculator_init(c);
        start=operand(c->expression);
        if(key==MC_DOT) {
            if(strchr(c->expression+start,'.') || strchr(c->expression+start,'e') || strchr(c->expression+start,'E')) return 0;
            n=strlen(c->expression);
            return append(c,(n==start || (n==start+1 && c->expression[start]=='-')) ? "0." : ".");
        }
        text[0]=(char)('0'+key);return append(c,text);
    }
    if(key==MC_SIGN) {
        if(c->evaluated) { c->value=-c->value;display_result(c);continue_result(c);c->evaluated=1;return 1; }
        start=operand(c->expression);n=strlen(c->expression);
        if(c->expression[start]=='-') memmove(c->expression+start,c->expression+start+1,n-start);
        else {
            if(n==MC_EXPRESSION_MAX) {c->error=MC_FULL;return 0;}
            memmove(c->expression+start+1,c->expression+start,n-start+1);c->expression[start]='-';
        }
        return 1;
    }
    if(key>=MC_ADD && key<=MC_DIV) {
        char symbol="+-*/"[key-MC_ADD],text[2]={0};
        if(c->evaluated) continue_result(c);
        n=strlen(c->expression);
        if(!n) return symbol=='-' ? append(c,"-") : 0;
        if(op(c->expression[n-1])) {
            if(symbol=='-' && c->expression[n-1]!='-') return append(c,"-");
            if(n==1) return 0;
            if(c->expression[n-1]=='-' && op(c->expression[n-2])) c->expression[--n]='\0';
            c->expression[n-1]=symbol;return 1;
        }
        text[0]=symbol;return append(c,text);
    }
    return 0;
}
const char *mc_error_name(McError e) {
    static const char *const names[]={"OK","EXPRESIE INCOMPLETA","DIVIZARE CU ZERO","NUMAR PREA MARE","EXPRESIE PREA LUNGA"};
    return (unsigned)e<=MC_FULL?names[e]:"EROARE";
}
static const char *const tokens[]={"0","1","2","3","4","5","6","7","8","9",".","+","-","*","/","=","C","DEL","SIGN","SET","BACK","MUTE","CAL","VOL+","VOL-"};
McKey mc_key_parse(const char *token) {
    unsigned i;if(!token)return MC_KEY_INVALID;
    if(!strcmp(token,",")) return MC_DOT;
    for(i=0;i<sizeof(tokens)/sizeof(tokens[0]);++i) if(!strcmp(token,tokens[i])) return (McKey)i;
    return MC_KEY_INVALID;
}
const char *mc_key_token(McKey key) { return (unsigned)key<MC_KEY_INVALID ? tokens[key] : "INVALID"; }
