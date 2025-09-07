---
tags:
  - command
---

# /rez

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/rez [ accept|safemode|voice|silent ] [on|off] | [ pct <#> ] | [ setcommand <command> ] | [ help|settings|release ]
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
Configures settings related to if and when you accept a rez, as well as notifications.
<!--cmd-desc-end-->

## Options

`(no option)`
:    displays settings

`accept [on|off]`
:    Toggle auto-accepting rezbox

`pct #`
:    Accepts rez only if it's higher than # percent

`safemode [on|off]`
:    Turn On/Off safe mode (Accepts rez from group/guild/fellowship)

`voice [on|off]`
:    Turns On/Off voice macro "Help" sound output when you die. This is local to you only.

`silent [on|off]`
:    Turn On/Off text output when receiving a rez.

`setcommand <"mycommand">`
:    Set the command that you want to run after rez

`help`
:    displays help text

`settings`
:    display current settings

`release`
:    Release to bind On/Off
