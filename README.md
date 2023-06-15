# Interpr-teur-de-Commande-Shell
Ce mini-projet consiste à développer un shell UNIX en langage C sans utiliser l'appel système system(). 
Le shell devra être capable d'exécuter des commandes en utilisant les appels systèmes open(), fork(), exec(), wait(), etc.
Fonctionnalités
Le shell devra prendre en charge les fonctionnalités suivantes :

Exécution de commandes internes (telles que cd, help, exit, etc.)
Exécution de commandes externes en utilisant les appels systèmes appropriés
Gestion des redirections d'entrée/sortie (redirection vers un fichier, redirection de la sortie standard vers l'entrée d'une autre commande, etc.)
Gestion des tubes (pipes) pour permettre la communication entre les commandes
Gestion des signaux pour interrompre ou arrêter l'exécution d'une commande
Comment compiler et exécuter
Pour compiler le code source du shell, vous pouvez utiliser la commande suivante :


gcc shell.c -o shell
Une fois le programme compilé, vous pouvez l'exécuter en utilisant la commande suivante :

./shell
Utilisation du shell
Le shell peut être utilisé de la même manière qu'un shell UNIX classique. 
Vous pouvez saisir des commandes et appuyer sur la touche "Entrée" pour les exécuter. 
Assurez-vous de respecter la syntaxe des commandes UNIX.

Notes importantes
Assurez-vous de ne pas utiliser l'appel système system() dans votre implémentation.
Veillez à gérer correctement les erreurs et à afficher des messages d'erreur appropriés lorsque nécessaire.
N'hésitez pas à ajouter des fonctionnalités supplémentaires ou à améliorer le shell selon vos besoins.
Ce projet a été réalisé dans le cadre d'un mini-projet en environnement UNIX et en langage C. Il a pour objectif de mettre en pratique les connaissances acquises sur les appels systèmes et le développement de shells.

Auteur : Mouhib MANI
Date : 15/11/2023

N'hésitez pas à personnaliser ce contenu en ajoutant des informations supplémentaires sur votre projet et en mettant à jour les sections en fonction de vos besoins.
