# PROG5 - Projet logiciel (groupe 5)

**Éditeur de liens - Phase de fusion**

L'objectif  de  ce  projet  est  d’implémenter  une  sous  partie  d'un  éditeur  de  liens. Plus  précisément, le projet est centré sur la première phase, dite de fusion, exécutée par l’éditeur de liens. Le projet est structuré en étapes, avec la programmation de plusieurs outils intermédiaires, permettant de mieux comprendre les principales notions et de simplifier le découpage des tâches.

***

## Membres du groupe

* Extrant Robin ([RobinExtrant](https://github.com/RobinExtrant))
* Omel Jocelyn ([Konemi](https://github.com/Konemi))
* Raynaud Paul ([plokier](https://github.com/plokier))
* Sorin Gaëtan ([Oloar](https://github.com/Oloar))
* Visage Yohan ([visagey](https://github.com/visagey))

## Liste des tâches

### Phase 1
- [X] Étape 1 : Affichage de l’en-tête
- [X] Étape 2 : Affichage de la table des sections
- [X] Étape 3 : Affichage du contenu d’une section
- [X] Étape 4 : Affichage de la table des symboles
- [X] Étape 5 : Affichage des tables de réimplantation

### Phase 2
- [X] Étape 6 : Fusion et renumérotation des sections
- [X] Étape 7 : Fusion, renumérotation et correction des symboles
- [X] Étape 8 : Fusion et correction des tables de réimplantations
- [ ] Étape 9 : Production d’un fichier résultat au format ELF

***

## Compilation & exécution

### Compilation
Il faut exécuter la séquence de commandes suivante :
```
$ mkdir build && cd $_ # On crée un répertoire dans lequel on se place
$ cmake .. # Invocation de CMake
$ make -j$(nproc)
```

### Exécution
Il suffit de se replacer dans le répertoire parent (`cd ..` par exemple).  
Puis il suffit d'exécuter l'un des fichiers binaires qui suit :

1. `$ ./readelf` : affiche des informations sur un fichier au format ELF (seule la classe ELF32 est supportée)
2. `$ ./fusion` : fusionne deux fichiers .o pour n'en créer plus qu'un

### Exemples d'utilisation
1. `$ ./readelf -h tests/hello.o`
2. `$ ./readelf -A -x1 -x .rodata tests/hello.o`
3. `$ ./fusion tests/file1.o tests/file2.o tests/prog.o`
