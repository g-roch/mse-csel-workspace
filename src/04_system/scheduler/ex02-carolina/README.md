# CGroups

## Exercice #2: Concevez une petite application permettant de valider la capacité des groupes de contrôle à limiter l’utilisation de la mémoire.

Quelques indications pour la création du programme :

1. Allouer un nombre défini de blocs de mémoire d’un mébibyte1, par exemple 50
2. Tester si le pointeur est non nul
3. Remplir le bloc avec des 0

Quelques indications pour monter les CGroups :

    ```sh
    $ mount -t tmpfs none /sys/fs/cgroup
    $ mkdir /sys/fs/cgroup/memory
    $ mount -t cgroup -o memory memory /sys/fs/cgroup/memory
    $ mkdir /sys/fs/cgroup/memory/mem
    $ echo $$ > /sys/fs/cgroup/memory/mem/tasks
    $ echo 20M > /sys/fs/cgroup/memory/mem/memory.limit_in_bytes
    ```

### Procédure de résolution de l'exercice

Créer un petit programme en C (ou autre) qui alloue des blocs de mémoire d’un mébibyte, les remplit de 0 et vérifie que les allocations ont réussi. Ce programme montre aussi la progression de l’utilisation de la mémoire.

Faire la configuration des cgroups à la main, en mettant le PID du shell qui lancera le programme (`echo $$ > /sys/fs/cgroup/memory/mem/tasks`) et en limitant la mémoire à différentes valeurs pour tester.

Voici quelques runs :

```sh
# echo 5M > /sys/fs/cgroup/memory/mem/memory.limit_in_bytes 
# ./build/cgroup_mem_tester 
Allocated 1 MiB
Allocated 2 MiB
Allocated 3 MiB
Allocated 4 MiB
Allocated 5 MiB
Killed
# echo 10M > /sys/fs/cgroup/memory/mem/memory.limit_in_bytes 
# ./build/cgroup_mem_tester 
Allocated 1 MiB
Allocated 2 MiB
Allocated 3 MiB
Allocated 4 MiB
Allocated 5 MiB
Allocated 6 MiB
Allocated 7 MiB
Allocated 8 MiB
Allocated 9 MiB
Allocated 10 MiB
Killed
```

### Questions
Quelques questions :

1. Quel effet a la commande echo $$ > ... sur les cgroups ?
> La commande `echo $$ > /sys/fs/cgroup/memory/mem/tasks` ajoute le processus actuel (identifié par son PID, obtenu avec `$$`) au groupe de contrôle de mémoire spécifié. Cela signifie que les limites de mémoire définies pour ce groupe s'appliqueront à ce processus.
> 
> Dans notre cas, on est en train d'ajouter le PID du shell dans lequel on est. Parce que les enfants héritent des limites de mémoire, notre programme va aussi être limité à 20 MiB.

2. Quel est le comportement du sous-système memory lorsque le quota de mémoire est épuisé ? Pourrait-on le modifier ? Si oui, comment ?

> Il tue le processus qui a dépassé la limite de mémoire. Oui, on peut le modifier en changeant la politique d'éviction (OOM killer) avec les fichiers `memory.oom_control` et `memory.oom_kill_disable`. En désactivant le OOM killer, le processus ne sera pas tué, mais il recevra des erreurs d'allocation de mémoire (ENOMEM) lorsqu'il essaiera d'allouer plus de mémoire que la limite définie.

3. Est-il possible de surveiller/vérifier l’état actuel de la mémoire ? Si oui, comment ?

> Oui, on peut vérifier l'état actuel de la mémoire en lisant le fichier `memory.usage_in_bytes`. Procédure:

1. Ouvrir un terminal, ajouter son PID
2. Ouvrir un autre terminal et exécuter la commande suivante pour surveiller l'utilisation de la mémoire en temps réel :

    ```bash
    while :; do cat /sys/fs/cgroup/memory/mem/memory.usage_in_bytes; sleep 1; done
    ```
    
3. Lancer le programme de test de mémoire dans le premier terminal et observer les changements dans l'utilisation de la mémoire dans le second terminal.

Résultat avec une limite de 25MiB:

```sh
# while :; do cat /sys/fs/cgroup/memory/mem/memory.usage_in_bytes; sleep 1; done
131072
131072
131072
131072
1351680
2400256
3448832
4497408
5545984
6725632
7774208
8822784
9871360
10919936
11968512
13017088
15114240
16162816
17211392
18259968
19308544
20357120
21405696
22454272
23502848
24551424
25600000
```
