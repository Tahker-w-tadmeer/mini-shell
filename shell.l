%{

#include <string.h>
#include "y.tab.h"

%}

%%

\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}	

">" 	{
		return GREAT;
	}
">>"    {
        return DOUBLEGREAT;
}
">>&"    {
        return DOUBLEGREATAND;
}
">&"    {
        return GREATAND;
}
"&"    {
        return BG;
}
"<" 	{
        return LESS;
    }

"|" {
    return PIPE;
}

[^ \t\n][^ \t\n]*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}

.	{
		/* Invalid character in input */
		return NOTOKEN;
	}

%%
	
