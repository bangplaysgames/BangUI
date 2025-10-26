
# BML Syntax Extension for VS Code

This extension provides syntax highlighting and language support for Bang Markup Language (BML) files in Visual Studio Code.

## Features

- Syntax highlighting for BML objects, properties, strings, constants, and comments
- Support for line comments (`// ...`) and block comments (`/* ... */`)
- Object and property markers are highlighted using globally themed scopes for best compatibility
- Folding and indentation rules for BML files

## BML Language Syntax

- **Objects**: Begin with `#` and end with `:`
	- Example: `#btnExit:`
- **Properties**: Begin with `>` and use a colon to separate name and value
	- Example: `>Text: "Exit"`
- **Line Comments**: Start with `//`
- **Block Comments**: Enclosed in `/* ... */`
- **Values**: Can be quoted strings or constants
- **Whitespace**: Completely ignored in BML—spacing and indentation do not affect parsing

### Example

```bml
//Line Comment

/* Block
	 Comment */

#btnExit: /* A button UI element */
>Text: "Exit" /* The Text property */
>TextColor: FF000000 /* The Text Color property */

#pnlMenu: /* A panel UI element */
>BackgroundColor: 303030FF /* Background Color */
>Width: 400 /* Width property */
>Height: auto /* Height property */
>hDock: Bottom /* Horizontal Docking */
>vDock: Fill /* Vertical Docking */
```

## Requirements

No special requirements. Just install and start editing `.bml` files!

## Known Issues

- None reported yet.

## Release Notes

### 1.0.0
- Initial release with full BML syntax support

---

**Enjoy using BML in VS Code!**
