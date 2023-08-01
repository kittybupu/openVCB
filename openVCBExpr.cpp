/*
 * Code for instruction expression parsing.
 */

// ReSharper disable CppClangTidyCertErr33C
#include "openVCB.h"
#include "openVCB_Expr.hh"


namespace openVCB
{
using SymMap = std::map<std::string, uint32_t>;

enum types : uint8_t { DELIMITER = 1, VARIABLE };

class parser {
      char *exp_ptr;    // points to the expression
      char  token[512]; // holds current token
      char  tok_type;   // holds token's type
      char  errormsg[128];

      SymMap &vars;

      void eval_expr1(int64_t &result);
      void eval_expr2(int64_t &result);
      void eval_expr3(int64_t &result);
      void eval_expr4(int64_t &result);
      void eval_expr5(int64_t &result);
      void eval_expr6(int64_t &result);
      void eval_expr7(int64_t &result);
      void eval_expr8(int64_t &result);
      void get_token();

      template <size_t SrcSize>
          requires(sizeof errormsg >= SrcSize)
      __inline constexpr void setError(char const (&src)[SrcSize])
      {
            ::memcpy(errormsg, src, SrcSize);
      }

      ATTRIBUTE_PRINTF(2, 3)
      int formatError(char const *fmt, ...);

public:
      explicit parser(SymMap &vars);
      int64_t  eval_expr(char *exp);

      ND char const *getError() const { return errormsg; }
      ND explicit    operator bool() const { return errormsg[0] == '\0'; }
};

// Parser constructor.
parser::parser(SymMap &vars)
      : exp_ptr(nullptr),
        tok_type(0),
        vars(vars)
{
      errormsg[0] = '\0';
      token[0]    = '\0';
}

// Parser entry point.
int64_t
parser::eval_expr(char *exp)
{
      errormsg[0] = '\0';
      int64_t result;
      exp_ptr = exp;
      get_token();
      if (!*token) {
            // No expression present.
            setError("empty");
            return 0;
      }
      eval_expr1(result);
      if (*token)
            // The last token must be null.
            formatError("syntax error (%c is not NUL)", *token);
      return result;
}

void
parser::eval_expr1(int64_t &result)
{
      int64_t temp;
      eval_expr2(result);

      while (*token == '|') {
            get_token();
            eval_expr2(temp);
            result = result | temp;
      }
}

void
parser::eval_expr2(int64_t &result)
{
      int64_t temp;
      eval_expr3(result);

      while (*token == '^') {
            get_token();
            eval_expr3(temp);
            result = result ^ temp;
      }
}

void
parser::eval_expr3(int64_t &result)
{
      int64_t temp;
      eval_expr4(result);

      while (*token == '&') {
            get_token();
            eval_expr4(temp);
            result = result & temp;
      }
}

void
parser::eval_expr4(int64_t &result)
{
      char    op;
      int64_t temp;
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
            default: ;
            }
      }
}

void
parser::eval_expr5(int64_t &result)
{
      char    op;
      int64_t temp;
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
            default: ;
            }
      }
}

// Multiply or divide two factors.
void
parser::eval_expr6(int64_t &result)
{
      char    op;
      int64_t temp;
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
            default: ;
            }
      }
}

void
parser::eval_expr7(int64_t &result)
{
      char op = 0;
      if (tok_type == DELIMITER && (*token == '!' || *token == '~' || *token == '+' || * token == '-')) {
            op = *token;
            get_token();
      }
      eval_expr8(result);
      switch (op) {
      case '-':
            result = -result;
            break;
      case '~':
            result = ~result;
            break;
      case '!':
            // MSVC complains unless three exclamation points are used.
            result = !!!result;
            break;
      default: ;
      }
}

// Process a function, a parenthesized expression, a value or a variable
void
parser::eval_expr8(int64_t &result)
{
      if (*token == '(') {
            get_token();
            eval_expr1(result);
            if (*token != ')')
                  setError("unbalanced parentheses");
            get_token();
      } else {
            if (isdigit(*token)) {
                  int const id = tolower(*(token + 1));
                  if (id == 'x') {
                        char const *ptr = token + 2;
                        result    = 0;
                        while (*ptr) {
                              int val = tolower(*ptr++);
                              if (val != '_') {
                                    val -= isdigit(val) ? '0' : 'a' - 10;
                                    result = (result << 4) | val;
                              }
                        }
                  } else if (id == 'b') {
                        char const *ptr = token + 2;
                        result    = 0;
                        while (*ptr) {
                              if (*ptr != '_')
                                    result = (result << 1) | (*ptr == '1');
                              ++ptr;
                        }
                  } else {
                        char *endp;
                        auto &e = errno;
                        e       = 0;
                        result  = ::strtoimax(token, &endp, 0);

                        if (endp && *endp != '\0')
                              formatError("Invalid character \"%c\" in number", *endp);
                        else if (result > UINT32_MAX)
                              formatError("Number \"%" PRId64 "\" is too large to fit in 32-bits", result);
                        else if (e == ERANGE)
                              setError("Number is out of range");
                  }

                  get_token();
            } else {
                  auto const &itr = vars.find(token);
                  if (itr == vars.end())
                        setError("undefined symbol");
                  else
                        result = itr->second;
                  get_token();
            }
      }
}

// Obtain the next token.
void
parser::get_token()
{
      static constexpr char search_term_1[] = "+-/%*|&^~!><()";
      static constexpr char search_term_2[] = "+-/%*|&^~!><() \t";

      char *temp = token;
      *temp      = '\0';

      if (!*exp_ptr) // at end of expression
            return;

      // Check if this is an implicit OR
      if (tok_type == VARIABLE && isspace(*exp_ptr)) {
            while (isspace(*(exp_ptr + 1)))
                  ++exp_ptr;
            if (!strchr(search_term_1, *(exp_ptr + 1))) {
                  *temp++ = '|';
                  *temp   = '\0';
                  ++exp_ptr;
                  return;
            }
      }

      // Skip over white space
      while (isspace(*exp_ptr))
            ++exp_ptr;

      tok_type = 0;
      if (strchr(search_term_2, *exp_ptr)) {
            tok_type = DELIMITER;
            *temp++  = *exp_ptr++;
            if (*exp_ptr == '<' || *exp_ptr == '>')
                  *temp++ = *exp_ptr++;
      } else {
            while (!strchr(search_term_2, *exp_ptr) && (*exp_ptr))
                  *temp++ = *exp_ptr++;

            tok_type = VARIABLE;
      }

      *temp = '\0';
}

int
parser::formatError(PRINTF_FORMAT_STRING fmt, ...)
{
      va_list ap;
      va_start(ap, fmt);
      int const ret = ::vsnprintf(errormsg, std::size(errormsg), fmt, ap);
      va_end(ap);
      return ret;
}

uint32_t
evalExpr(char const *expr, SymMap &symbols, char *errp, size_t const errSize)
{
      parser        p(symbols);
      int64_t const res = p.eval_expr(const_cast<char *>(expr));

      if (!p) {
            if (errp)
                  ::snprintf(errp, errSize, "%s at %s", p.getError(), expr);
            else
                  ::fprintf(stderr, "error: \"%s\" %s\n", expr, p.getError());
      }

      return static_cast<uint32_t>(res);
}
} // namespace openVCB
