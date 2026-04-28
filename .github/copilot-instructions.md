- Use `make dev` after modification to check for build errors
- The project syntax rules are governed by `.clangd`, the readability rules must be respected:
```
readability-braces-around-statements

readability-identifier-naming.DefaultCase: camelBack
readability-identifier-naming.ClassCase: CamelCase
readability-identifier-naming.ConstantCase: UPPER_CASE
readability-identifier-naming.EnumCase: CamelCase
readability-identifier-naming.TemplateParameterCase: CamelCase
readability-identifier-naming.NamespaceCase: lower_case
readability-identifier-naming.PrivateMemberSuffix: _
```
- Class getters and setters should have the same name and rely on onverloading, for example, give a private member `name_`, its getter would be `const std::string & name()` and the setter would be `void name(const std::string &name)`
