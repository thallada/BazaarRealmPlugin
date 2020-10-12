# BazaarRealmPlugin

The SKSE plugin for the Bazaar Realm Skyrim mod.

## Building

[Follow the CommonLibSSE set-up
instructions](https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/wiki/Getting-Started#commonlibsse).
Instead of cloning the example plugin, clone this repo instead.

This plugin relies on a separate DLL plugin called `BazaarRealmClient`. To build
this plugin, it needs to be linked to it at compile-time. Checkout
`BazaarRealmClient` in a place of your choosing, build it, and then set the
environment variable `BazaarRealmClientPath` to the path you chose.

Building the plugin in Visual Studio will automatically place the built plugin
DLL in the `Skyrim Special Edition\Data\SKSE\Plugins\` folder.
