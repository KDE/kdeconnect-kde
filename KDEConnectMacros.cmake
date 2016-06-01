# Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
# Redistribution and use is allowed according to the terms of the BSD license.


# Thoroughly inspired in kdevplatform_add_plugin
function(kdeconnect_add_plugin)
    kcoreaddons_add_plugin(${ARGN} INSTALL_NAMESPACE kdeconnect)
endfunction()
