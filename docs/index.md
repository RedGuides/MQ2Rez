---
tags:
  - plugin
resource_link: "https://www.redguides.com/community/resources/mq2rez.180/"
support_link: "https://www.redguides.com/community/threads/mq2rez.66882/"
repository: "https://github.com/RedGuides/MQ2Rez"
config: "<server>_<character>.ini"
authors: "ChatWithThisName, Knightly, brainiac, TheZ, dewey, s0rcier, jimbob, eqmule"
tagline: "Will assist you by accepting rez based on INI settings."
acknowledgements: "Wait4Rez.mac and dewey's MQ2AutoRez"
---

# MQ2Rez

<!--desc-start-->
A plugin that will accept resurrections based on any conditions you set. Inspired by Wait4Rez.mac.
<!--desc-end-->

## Commands

<a href="cmd-rez/">
{% 
  include-markdown "projects/mq2rez/cmd-rez.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2rez/cmd-rez.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2rez/cmd-rez.md') }}

## Settings

Example MQ2Rez section from `<server>_<character>.ini`

```ini
[MQ2Rez]
Accept=On
;auto-accept reaz
RezPct=90
;only accept if the rez % is higher than this amount
SafeMode=On
;only accept from group/guild/fellowship
VoiceNotify=Off
;local "help!" sound when you die
ReleaseToBind=Off
;immediate release to bindpoint upon death
SilentMode=Off
;output text when recieving a rez
Command Line=DISABLED
;command to run after rez, accepts multiline.
Delay=0
;delay before accepting rez
```

## Top-Level Objects

## [Rez](tlo-rez.md)
{% include-markdown "projects/mq2rez/tlo-rez.md" start="<!--tlo-desc-start-->" end="<!--tlo-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2rez/tlo-rez.md') }}

## DataTypes

## [Rez](datatype-rez.md)
{% include-markdown "projects/mq2rez/datatype-rez.md" start="<!--dt-desc-start-->" end="<!--dt-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2rez/datatype-rez.md') }}

<h2>Members</h2>
{% include-markdown "projects/mq2rez/datatype-rez.md" start="<!--dt-members-start-->" end="<!--dt-members-end-->" %}
{% include-markdown "projects/mq2rez/datatype-rez.md" start="<!--dt-linkrefs-start-->" end="<!--dt-linkrefs-end-->" %}
