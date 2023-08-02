# Projet Système - Threads en espace utilisateur

Projet académique ayant pour but de réaliser en équipe une bibliothèque C pour gérer des threads (s'exécutant sur un seul thread noyau).
Notre équipe est : it202-19001 (Thor) ou G4-E5 (Groupe)

[Lien vers la page du sujet](https://goglin.gitlabpages.inria.fr/enseirb-it202/)

[Lien vers le leaderboard/benchmark en ligne](https://goglin.gitlabpages.inria.fr/enseirb-it202-leaderboard/)

[Lien vers le depôt Thor](https://thor.enseirb-matmeca.fr/ruby/repositories/8611)

## Compilation

Pour la compilation de tout le projet avec la bibliothèque de threads du groupe (avec allocation dynamique)
```bash
$ make
ou
$ make all
```

Pour la compilation de tout le projet avec la bibliothèque de threads du groupe (avec allocation statique)
```bash
$ make static
```

Pour la compilation de tout le projet avec la bibliothèque pthreads
```bash
$ make pthreads
```


## Utilisation

Pour l'execution de l'ensemble des tests
```bash
$ make check
```

Pour l'execution de l'ensemble des tests avec valgrind
```bash
$ make valgrind
```
*Les sorties standard et d'erreur lors de l'execution des tests sont redirigées dans les fichiers `<nom_du_test>.txt`*


Pour l'expérimentation et l'obtention de graphiques sur la performance (temporelle) des différentes versions
```bash
$ make graphs
```
*Les graphiques sont récuperables dans le dossier `graphs` en format png*


Pour supprimer les exécutables, fichiers de compilation, resultats des tests et dossiers d'installation
```bash
$ make clean
```