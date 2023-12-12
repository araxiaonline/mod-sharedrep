# mod-sharedrep

This module is a database hack to do shared rep across an account in AzerothCore. It works by using some SQL magic (that might be wrong) to find the highest rep for all characters->factions in the account and then increases the current characters faction rep for each if it's lower. It does this at login only.

## Installation

Drop `mod-sharedrep` in your `modules/` folder, CMake, and build. Let the magic happen.

## Config

Debug logging can be enabled if you are having trouble. The enable/disable flag isn't enabled in code yet.

Questions, comments, concerns? Create an issue on our Github!
