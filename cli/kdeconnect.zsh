#compdef kdeconnect-cli

_kdeconnect_device-ids() {
  local devices=''
  devices="$(kdeconnect-cli --shell-device-autocompletion=zsh 2>/dev/null)"
  if [[ $? -eq 0 ]]; then
    _values "KDE Connect device id" "${(f)devices}"
  else
    _message -r "No KDE Connect devices reachable."
  fi
}

#See http://zsh.sourceforge.net/Doc/Release/Completion-System.html#index-_005farguments for autocomplete documentation
#
#The --desktopfile option is not included, since it rarely makes sense to use
#The --shell-device-autocompletion option is not included, since it's not even in the help (and only required for scripts like these)
local blockoperations="(operation sms)"
_arguments -s \
  '(-)'{-h,--help}'[display usage information]' \
  '(-)--help-all[display usage information, including Qt specific options]' \
  + '(global)' \
  $blockoperations{-l,--list-devices}'[list all devices]' \
  $blockoperations{-a,--list-available}'[list available (paired and reachable) devices]' \
  $blockoperations'--refresh[search for devices in the network and re-establish connections]' \
  {-d=,--device=}'[specify device ID]:id:_kdeconnect_device-ids' \
  {-n=,--name=}'[specify device name]:name' \
  $blockoperations''{-v,--version}'[display version information]' \
  $blockoperations'--author[show author information and exit]' \
  $blockoperations'--license[show license information and exit]' \
  $blockoperations"--my-id[display this device's id]" \
  + '(operation)' \
  $blockoperations'--pair[request pairing with the specified device]' \
  $blockoperations'--ring[find the device by ringing it.]' \
  $blockoperations'--unpair[stop pairing to the specified device]' \
  $blockoperations'--ping[send a ping to the device]' \
  $blockoperations'--ping-msg=[send a ping to the device with the specified message]:message' \
  $blockoperations'--share=[share a file to the device]:file:_files' \
  $blockoperations'--list-notifications[display the notifications on the device]' \
  $blockoperations'--lock[lock the specified device]' \
  $blockoperations'--encryption-info[get encryption info about the device]' \
  $blockoperations'--list-commands[list remote commands and their ids]' \
  $blockoperations'--execute-command=[execute a remote command]:command id' \
  $blockoperations{-k=,--send-keys=}'[send keyboard input to the specified device]:keyboard input' \
  $blockoperations"--photo=[open the connected device's camera and transfer the photo]:file:_files" \
  + 'sms' \
  '(operation)--send-sms=[send an SMS]:message:' \
  '(operation)--destination=[specify phone number to send the SMS to]:phone number:' \
  '(operation)*--attachment=[attachment to send with the message]:file:_files'
