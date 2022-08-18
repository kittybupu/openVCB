#include "openVCBExpr.h"

#include <iostream>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <string>
#include <unordered_map>

using namespace std;

enum types { DELIMITER = 1, VARIABLE };
const int NUMVARS = 26;
class parser {
	char* exp_ptr; // points to the expression
	char token[512]; // holds current token
	char tok_type; // holds token's type
	std::unordered_map<std::string, long long>& vars;
	void eval_expr1(long long& result);
	void eval_expr2(long long& result);
	void eval_expr3(long long& result);
	void eval_expr4(long long& result);
	void eval_expr5(long long& result);
	void eval_expr6(long long& result);
	void eval_expr7(long long& result);
	void eval_expr8(long long& result);
	void get_token();
public:
	parser(std::unordered_map<std::string, long long>& vars);
	long long eval_expr(char* exp);
	char errormsg[64];
};

// Parser constructor.
parser::parser(std::unordered_map<std::string, long long>& vars) : vars(vars) {
	int i;
	exp_ptr = NULL;
	errormsg[0] = '\0';
}
// Parser entry point.
long long parser::eval_expr(char* exp) {
	errormsg[0] = '\0';
	long long result;
	exp_ptr = exp;
	get_token();
	if (!*token) {
		strcpy_s(errormsg, "empty"); // no expression present
		return 0;
	}
	eval_expr1(result);
	if (*token) // last token must be null
		strcpy_s(errormsg, "syntax error");
	return result;
}

void parser::eval_expr1(long long& result) {
	register char op;
	long long temp;
	eval_expr2(result);
	while ((op = *token) == '|') {
		get_token();
		eval_expr2(temp);
		result = result | temp;
	}
}

void parser::eval_expr2(long long& result) {
	register char op;
	long long temp;
	eval_expr3(result);
	while ((op = *token) == '^') {
		get_token();
		eval_expr3(temp);
		result = result ^ temp;
	}
}

void parser::eval_expr3(long long& result) {
	register char op;
	long long temp;
	eval_expr4(result);
	while ((op = *token) == '&') {
		get_token();
		eval_expr4(temp);
		result = result ^ temp;
	}
}

void parser::eval_expr4(long long& result) {
	register char op;
	long long temp;
	eval_expr5(result);
	while ((op = *token) == '>' || op == '<') {
		get_token();
		eval_expr5(temp);
		switch (op) {
		case '>':
			result = result >> temp;
			break;
		case '<':
			result = result << temp;
			break;
		}
	}
}

void parser::eval_expr5(long long& result) {
	register char op;
	long long temp;
	eval_expr6(result);
	while ((op = *token) == '+' || op == '-') {
		get_token();
		eval_expr6(temp);
		switch (op) {
		case '-':
			result = result - temp;
			break;
		case '+':
			result = result + temp;
			break;
		}
	}
}

// Multiply or divide two factors.
void parser::eval_expr6(long long& result) {
	register char op;
	long long temp;
	eval_expr7(result);
	while ((op = *token) == '*' || op == '/' || op == '%') {
		get_token();
		eval_expr7(temp);
		switch (op) {
		case '*':
			result = result * temp;
			break;
		case '/':
			result = result / temp;
			break;
		case '%':
			result = result % temp;
			break;
		}
	}
}

void parser::eval_expr7(long long& result) {
	register char op;
	op = 0;
	if ((tok_type == DELIMITER) &&
		*token == '!' || *token == '~' || *token == '+' || *token == '-') {
		op = *token;
		get_token();
	}
	eval_expr8(result);
	if (op == '-')
		result = -result;
	else if (op == '~')
		result = ~result;
	else if (op == '!')
		result = !result;
}

// Process a function, a parenthesized expression, a value or a variable
void parser::eval_expr8(long long& result) {
	char temp_token[80];
	if ((*token == '(')) {
		get_token();
		eval_expr1(result);
		if (*token != ')')
			strcpy_s(errormsg, "unbalanced parentheses");
		get_token();
	}
	else {
		if (isdigit(*token)) {
			const char id = tolower(*(token + 1));
			if (id == 'x') {
				char* ptr = token + 2;
				result = 0;
				while (*ptr) {
					int val = tolower(*ptr++);
					if (val != '_') {
						val -= isdigit(val) ? '0' : 'a' - 10;
						result = (result << 4) | val;
					}
				}
			}
			else if (id == 'b') {
				char* ptr = token + 2;
				result = 0;
				while (*ptr) {
					if (*ptr != '_')
						result = (result << 1) | (*ptr == '1');
					ptr++;
				}
			}
			else
				result = atoll(token);
			get_token();
		}
		else {
			auto itr = vars.find(token);
			if (itr == vars.end())
				strcpy_s(errormsg, "undefined symbol");
			else
				result = itr->second;
			get_token();
		}
	}
}

// Obtain the next token.
void parser::get_token() {
	char* temp;
	temp = token;
	*temp = '\0';

	if (!*exp_ptr)  // at end of expression
		return;

	// Check if this is an implicit or
	if (tok_type == VARIABLE && isspace(*exp_ptr)) {
		while (isspace(*(exp_ptr + 1)))
			++exp_ptr;
		if (!strchr("+-/%*|&^~!><)", *(exp_ptr + 1))) {
			*temp++ = '|';
			*temp = '\0';
			exp_ptr++;
			return;
		}
	}

	// Skip over white space
	while (isspace(*exp_ptr))
		++exp_ptr;

	tok_type = 0;
	if (strchr("+-/%*|&^~!><() ", *exp_ptr)) {
		tok_type = DELIMITER;
		*temp++ = *exp_ptr++;
		if (*exp_ptr == '<' || *exp_ptr == '>')
			*temp++ = *exp_ptr++;
	}
	else {
		while (!strchr("+-/%*|&^~!><() \t", *exp_ptr) && (*exp_ptr))
			*temp++ = *exp_ptr++;
		tok_type = VARIABLE;
	}
	*temp = '\0';
}

long long openVCB::evalExpr(const char* expr,
	std::unordered_map<std::string, long long>& symbols) {
	parser p(symbols);
	auto res = p.eval_expr((char*)expr);
	if (*p.errormsg)
		printf("error: \"%s\" %s\n", expr, p.errormsg);
	return res;
}
