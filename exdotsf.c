/*
    EXDotSF, an interpreter implementing an extension of ArthroStar11's DotSF esolang.
    exdotsf.c is written by Rudolph#4268 (https://github.com/SIGSEGV-666 || https://esolangs.org/wiki/User:Rudolph4268 || fzerowipeoutlover1998@gmail.com)
    DISCLAIMER: This does not use any code from ArthroStar11's original DotSF interpreter. This implementation has been completely written from scratch by me.
    
    esolangs.org wiki page for EXDotSF: https://esolangs.org/wiki/EXDotSF

BSD 3-Clause License

Copyright (c) 2021, SIGSEGV-666
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#define DOTSF_MAX_STACKS 10
#define DOTSF_MAX_STACK_SIZE 30000
#define DOTSF_MAX_HASHOP_VAL_SIZE 65
typedef int dotsf_int;
typedef struct {
    bool in_use;
    dotsf_int *stack, stacktop, maxstack;
} dotsf_stack;
typedef struct {
    dotsf_stack stacks[DOTSF_MAX_STACKS];
    unsigned int curstack;
} dotsf_interpreter;
bool _dotsf_push_to_stack(dotsf_interpreter* interp, dotsf_int stacknum, dotsf_int value)
{
    if (stacknum < 0 || stacknum >= DOTSF_MAX_STACKS){return false;}
    dotsf_stack* stack = interp->stacks+stacknum;
    if (!stack->in_use){return false;}
    if (stack->stacktop < 0){stack->stack[0] = value; stack->stacktop = 0; return true;}
    else if (stack->stacktop >= 0 && stack->stacktop <= stack->maxstack){stack->stack[++(stack->stacktop)] = value; return true;} 
    return false;   
}
bool _dotsf_push(dotsf_interpreter* interp, dotsf_int value)
{
    return _dotsf_push_to_stack(interp, interp->curstack, value);
}
bool _dotsf_pop_from_stack(dotsf_interpreter* interp, dotsf_int stacknum, dotsf_int* ret)
{
    if (stacknum < 0 || stacknum >= DOTSF_MAX_STACKS){return false;}
    dotsf_stack* stack = interp->stacks+stacknum;
    if (!stack->in_use){return false;}
    if (stack->stacktop >= 0){*ret = stack->stack[(stack->stacktop)--]; return true;}
    return false;
}
bool _dotsf_pop(dotsf_interpreter* interp, dotsf_int* ret)
{
    return _dotsf_pop_from_stack(interp, interp->curstack, ret);
}
bool _dotsf_popbottom(dotsf_interpreter* interp, dotsf_int stacknum, dotsf_int* ret)
{
    if (stacknum < 0 || stacknum >= DOTSF_MAX_STACKS){return false;}
    dotsf_stack* stack = interp->stacks+stacknum;
    if (stack->stacktop >= 0)
    {
        *ret = stack->stack[0];
        for (dotsf_int i = 1; i <= stack->stacktop; i++){stack->stack[i-1] = stack->stack[i];} stack->stacktop--;
        return true;
    }
    return false;
}
bool _dotsf_popb_pusht(dotsf_interpreter* interp, dotsf_int stacknum)
{
    dotsf_int v;
    if (!_dotsf_popbottom(interp, stacknum, &v)){return false;}
    if (!_dotsf_push_to_stack(interp, stacknum, v)){return false;}
    return true;
}
bool _dotsf_gettop(dotsf_interpreter* interp, dotsf_int stacknum, dotsf_int* out)
{
    if (stacknum < 0 || stacknum >= DOTSF_MAX_STACKS){return false;}
    dotsf_stack* stack = interp->stacks+stacknum;
    if (stack->stacktop < 0){return false;}
    *out = stack->stack[stack->stacktop];
    return true;
}
bool _dotsf_getbottom(dotsf_interpreter* interp, dotsf_int stacknum, dotsf_int* out)
{
    if (stacknum < 0 || stacknum >= DOTSF_MAX_STACKS){return false;}
    dotsf_stack* stack = interp->stacks+stacknum;
    if (stack->stacktop < 0){return false;}
    *out = stack->stack[0];
    return true;
}
int _dotsf_delete_stack(dotsf_interpreter* interp, dotsf_int stacknum)
{
    if (stacknum < 0 || stacknum >= DOTSF_MAX_STACKS){return 1;}
    dotsf_stack* stack = interp->stacks+stacknum;
    if (stack->in_use)
    {
        free(stack->stack);
        stack->stack = NULL;
        stack->maxstack = 0;
        stack->stacktop = -1;
        stack->in_use = false;
        return 0;
    }
    return 2;
}
int _dotsf_create_stack(dotsf_interpreter* interp, dotsf_int stacknum, dotsf_int maxstack, dotsf_int* outindex)
{
    dotsf_stack* stack = NULL;
    if (stacknum < 0)
    {
        for (unsigned int i = 0; i < DOTSF_MAX_STACKS; i++)
        {
            stack = interp->stacks+i;
            if (!stack->in_use)
            {
                stacknum = i;
                goto foundslot;
            }
        }
        return 1;
    }
    else {goto foundslot;}
    foundslot:
        if (stacknum >= DOTSF_MAX_STACKS){return 2;}
        stack = (interp->stacks+stacknum);
        if (stack->in_use){return 3;}
        if (outindex != NULL){*outindex = stacknum;}
        stack->maxstack = maxstack;
        stack->stacktop = -1;
        stack->stack = malloc(sizeof(dotsf_int)*maxstack);
        stack->in_use = true;
        if (!_dotsf_push(interp, stacknum)){return 4;}
        return 0;
}
int dotsf_exec(dotsf_interpreter* interp, char* src)
{
    #define _dotsf_modulus(a, b) ((a < 0) ? (b) : ((__typeof__(b))0))+(a-(b*((__typeof__(a))(ssize_t)((double)a/(double)b))))
    char* labels[26] = { };
    char* hashopend = NULL;
    char hashopval[DOTSF_MAX_HASHOP_VAL_SIZE] = { };
    dotsf_int _snum1, v1, v2, v3, rank1, ierank1;
    dotsf_stack* stack = NULL;
    ierank1 = 0;
    int count = 0;
    for (unsigned int si = 0; si < DOTSF_MAX_STACKS; si++)
    {
        _dotsf_delete_stack(interp, si);
    }
    _dotsf_create_stack(interp, 0, DOTSF_MAX_STACK_SIZE, NULL);
    _dotsf_pop(interp, &_snum1);
    for (char* ip2 = src; *ip2; ip2++)
    {
        if (*ip2 == '#')
        {
            if (*++ip2 == 'c'){ip2+=2;}
            else
            {
                ip2 = strchr(ip2, '\\');
                if (ip2 == NULL){return -50;}
            }
        }
        if (*ip2 >= 'A' && *ip2 <= 'Z'){labels[(*ip2)-'A'] = ip2;}
    }
    for (char* ip = src; *ip; ip++)
    {
        if (*ip >= 'a' && *ip <= 'z'){ip = labels[(*ip)-'a'];}
        else if (*ip == '[')
        {
            if (!_dotsf_pop(interp, &v1)){return -10;}
            else if (!v1)
            {
                ip++;
                for (rank1 = 1; ; ip++)
                {
                    switch (*ip)
                    {
                        case '[': rank1++; break;
                        case ']': rank1--; break;
                        case '\0': return -1;
                        default: break;
                    }
                    if (rank1 == 0){break;}
                }
            }
            else {;}    
        }
        else if (*ip >= '0' && *ip <= '9'){if (!_dotsf_push(interp, (*ip)-'0')){return -13;}}
        else if (*ip == ':'){if (!_dotsf_pop(interp, &v1)){return -10;} printf("%i\n", v1);}
        else if (*ip == ';'){if (!_dotsf_pop(interp, &v1)){return -10;} putchar(v1);}
        else if (*ip == '+')
        {
            if (!_dotsf_pop(interp, &v2)){return -14;}
            else if (!_dotsf_pop(interp, &v1)){return -15;}
            else if (!_dotsf_push(interp, v1+v2)){return -16;}
            else {;}
        }
        else if (*ip == '-')
        {
            if (!_dotsf_pop(interp, &v2)){return -14;}
            else if (!_dotsf_pop(interp, &v1)){return -15;}
            else if (!_dotsf_push(interp, v1-v2)){return -16;}
            else {;}
        }
        else if (*ip == '*')
        {
            if (!_dotsf_pop(interp, &v2)){return -14;}
            else if (!_dotsf_pop(interp, &v1)){return -15;}
            else if (!_dotsf_push(interp, v1*v2)){return -16;}
            else {;}
        }
        else if (*ip == '/')
        {
            if (!_dotsf_pop(interp, &v2)){return -14;}
            else if (!_dotsf_pop(interp, &v1)){return -15;}
            else if (!_dotsf_push(interp, v1/v2)){return -16;}
            else {;}
        }
        else if (*ip == '%')
        {
            if (!_dotsf_pop(interp, &v2)){return -14;}
            else if (!_dotsf_pop(interp, &v1)){return -15;}
            else if (!_dotsf_push(interp, _dotsf_modulus(v1, v2))){return -16;}
            else {;}
        }
        else if (*ip == '=')
        {
            if (!_dotsf_pop(interp, &v2)){return -14;}
            else if (!_dotsf_pop(interp, &v1)){return -15;}
            else if (!_dotsf_push(interp, (dotsf_int)(v1 == v2))){return -16;}
            else {;}
        }
        else if (*ip == '>')
        {
            if (!_dotsf_pop(interp, &v2)){return -14;}
            else if (!_dotsf_pop(interp, &v1)){return -15;}
            else if (!_dotsf_push(interp, (dotsf_int)(v1 > v2))){return -16;}
            else {;}
        }
        else if (*ip == '<')
        {
            if (!_dotsf_pop(interp, &v2)){return -14;}
            else if (!_dotsf_pop(interp, &v1)){return -15;}
            else if (!_dotsf_push(interp, (dotsf_int)(v1 < v2))){return -16;}
            else {;}
        }
        else if (*ip == '&')
        {
            if (!_dotsf_pop(interp, &v2)){return -14;}
            else if (!_dotsf_pop(interp, &v1)){return -15;}
            else if (!_dotsf_push(interp, (dotsf_int)(v1 && v2))){return -16;}
            else {;}
        }
        else if (*ip == '{')
        {
            if (!_dotsf_pop(interp, &v2)){return -14;}
            else if (!_dotsf_pop(interp, &v1)){return -15;}
            else if (!_dotsf_push(interp, (dotsf_int)(v1 <= v2))){return -16;}
            else {;}
        }
        else if (*ip == '}')
        {
            if (!_dotsf_pop(interp, &v2)){return -14;}
            else if (!_dotsf_pop(interp, &v1)){return -15;}
            else if (!_dotsf_push(interp, (dotsf_int)(v1 >= v2))){return -16;}
            else {;}
        }
        else if (*ip == '.')
        {
            v1 = 0;
            if (!scanf("%i", &v1)){return -17;}
            else if (!_dotsf_push(interp, v1)){return -18;}
            else {;}
        }
        else if (*ip == ',')
        {
            v1 = getchar();
            if (v1 == EOF){v1 = 0;}
            if (!_dotsf_push(interp, v1)){return -19;}
            else {;}
        }
        else if (*ip == '_')
        {
            if (!_dotsf_pop(interp, &v1)){return -20;}
            for (int i = 0; i < 2; i++){if (!_dotsf_push(interp, v1)){return -(21+i);}}
        }
        else if (*ip == '@')
        {
            if (!_dotsf_pop(interp, &v2)){return -23;}
            else if (!_dotsf_pop(interp, &v1)){return -24;}
            for (int i = 0; i < 4; i++){if (!_dotsf_push(interp, (i%2) ? (v2) : (v1))){return -(25+i);}}
        }
        else if (*ip == '?')
        {
            if (!_dotsf_pop(interp, &v1)){return -30;}
            if (!v1)
            {
                ip++;
                for (rank1 = 1; ; ip++)
                {
                    switch (*ip)
                    {
                        case '?': rank1++; break;
                        case '|': rank1--; break;
                        default: break;
                    }
                    if (rank1 == 0){break;}
                }
            }
            else
            {
                ierank1++;
            }
        }
        else if (*ip == '|' && ierank1 > 0)
        {
            ierank1--;
            for (rank1 = 1; ; ip++)
            {
                switch(*ip)
                {
                    case '|': rank1++; break;
                    case '\'': rank1--; break;
                    default: break;
                }
                if (rank1 == 0){break;}
            }
        }
        else if (*ip == '"')
        {
            while (true)
            {
                v1 = getchar();
                if (v1 == EOF || v1 == '\r' || v1 == '\n'){break;}
                if (!_dotsf_push(interp, v1)){return -31;}
            }
            if (!_dotsf_push(interp, 0)){return -32;}
        }
        else if (*ip == '~'){if (!_dotsf_popb_pusht(interp, interp->curstack)){return -33;}}
        else if (*ip == '#')
        {
            ip++;
            switch (*ip)
            {
                case 'c': if (!_dotsf_push(interp, *++ip)){return -52;} break;
                case 'n':
                    hashopend = strchr(ip, '\\');
                    if (hashopend-(ip+1) >= DOTSF_MAX_HASHOP_VAL_SIZE){return -51;}
                    memset(hashopval, 0, DOTSF_MAX_HASHOP_VAL_SIZE);
                    memcpy(hashopval, ip+1, hashopend-(ip+1));
                    if (!sscanf(hashopval, "%i", &v1)){return -53;}
                    if (!_dotsf_push(interp, v1)){return -54;}
                    ip = hashopend;
                    break;
                default: return -60;

            }
        }
        else if (*ip == '`')
        {
            stack = interp->stacks+interp->curstack;
            puts("\nTHE CURRENT STACK IS:\n");
            for (dotsf_int sii = 0; sii <= stack->stacktop; sii++)
            {
                printf("%i = %i\n", sii, stack->stack[sii]);
            }
            puts("");
        }
        else {;}
    }
    return 0;
    #undef _dotsf_modulus
}

dotsf_interpreter INTERP = { };
int main(int argc, char** argv)
{

    char* program_src = NULL;
    FILE* fp = NULL;
    size_t fsize = 0;
    if (argc < 2){puts("ERROR: At least 1 command line argument is required."); return -555;}
    fp = fopen(argv[1], "rb");
    if (fp == NULL){printf("ERROR: No file named %s\n", argv[1]); return -666;}
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    program_src = calloc(fsize+1, 1);
    fread(program_src, 1, fsize, fp);
    fclose(fp);

    int res = dotsf_exec(&INTERP, program_src);
    if (res < 0){printf("Error Status %i\n", res); return res;}
    return res;
}