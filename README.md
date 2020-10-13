# BazaarRealmPlugin

The SKSE plugin for the Bazaar Realm Skyrim mod.

Related projects:

* [`BazaarRealmAPI`](https://github.com/thallada/BazaarRealmAPI): API server
  for the mod that stores all shop data
* [`BazaarRealmClient`](https://github.com/thallada/BazaarRealmClient): DLL that
  handles requests and responses to the API
* [`BazaarRealmMod`](https://github.com/thallada/BazaarRealmMod): Papyrus
  scripts, ESP plugin, and all other resources for the mod

## Building

[Follow the CommonLibSSE set-up
instructions](https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/wiki/Getting-Started#commonlibsse).
Instead of cloning the example plugin, clone this repo instead.

This plugin relies on a separate DLL plugin called `BazaarRealmClient`. To build
this plugin, it needs to be linked to it at compile-time. Checkout
[`BazaarRealmClient`](https://github.com/thallada/BazaarRealmClient) in a place
of your choosing, build it, and then set the environment variable
`BazaarRealmClientPath` to the path you chose.

Building the plugin in Visual Studio will automatically place the built plugin
DLL in the `Skyrim Special Edition\Data\SKSE\Plugins\` folder.
