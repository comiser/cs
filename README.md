# c-short
 1. *program* → *decList* | **ε**
 2. *decList* → *decList* *dec* | *dec*
 3. *dec* → *vardec* | *funcdec*
 4. *vardec* → **var** *type-id* *vardecid* [= *simpleexpr*] **;**
 7. *vardecid* → **ID** | **ID** **[** NUMCONST **]**
 9. *funcdec* → **def**  **ID** **(** *params* **)** *type-id* **{** *locVarDecList* *stmtlist* **}**
 10. *params* → *paramlist*  | **ε**
 11. *paramlist* → *paramlist* **,** *paramtypelist* | *paramtypelist*
 12. *paramtypelist* → *type-id* *paramId*
 13. *paramid* → **ID** 
 14. *stmt* → *exprstmt* |  *compoundstmt*  | *ifstmt* | *whilestmt* | *forstmt* | *returnstmt* | *breakstmt*
 15. *compoundstmt*→ **{** *locVarDecList* *stmtlist* **}**
 16. *locVarDecList → locVarDecList LocVarDec* | *LocVarDec* 
 17. *LocVarDec* → **var** *type-id* *varDeclId* [= *simpleExpression*] **;**
 20. *stmtlist → stmtlist statement* | **ε** 
 21. *expressionStmt* → *exprlist* **;** | **;** 
 22. *ifstmt* → **if** **(** *exprlist* **)** *stmt* **[** **else** *stmt* **]**
 23. *whilestmt* → **while** **(** *exprlist* **)** *stmt*
 24. *forstmt* → **for** **(** *exprlist* **;** *exprlist* **;** *exprlist* **)** *stmt*
 24. *returnstmt* → **return** **;** | **return** *exprlist* **;**
 25. *breakstmt* → **break** **;**
 25. *exprlist* → *exprlist* **,** *expr* | *expr*
 26. *expr* → *assignexpr* | *simpleexpr*
 26. *assignexpr* → **$** *mutable* **=** *expr* 
 27. *simpleexpr → simpleexpr* **|** *andexpr* | *andexpr*
 28. *andexpr → andexpr* **&** *unaryRelExpr* | *unaryRelExpr*
 29. *unaryRelExpr →* **!** *relExpr* | *relExpr*
 30. *relExpr → sumExpr relop sumExpr* | *sumExpr*
 31. *relop* → **<=** | **<** | **>** | **>=** | **==** | **!=**
 32. *sumExpr → sumExpr sumop term* | *term*
 33. *sumop* → **+** | **−**
 34. *term → term mulop unaryexpr* | *unaryexpr*
 35. *mulop* → **'*'** | **/** | **%**
 36. *unaryexpr* → * **−** unaryexpr* | *factor*
 38. *factor* → *immutable* | *mutables*
 39. *mutable* → **ID**
 40. *immutable* → **(** *expr* **)** | *call* | NUMCONST | CHARCONST | STRINGCONST | **true** | **false**
 41. *call* → **[** *arglist* **]** **ID** 
 43. *arglist → arglist*  **','**  *expr* | *expr*
