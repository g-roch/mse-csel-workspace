#import "./src/lib.typ": report
#import "@preview/mitex:0.2.6": mitex
#import "@preview/cmarker:0.1.8"

#set text(lang: "fr")

#show: report.with(
  title: [Environnement Linux embarqué et programmation noyau Linux],
  subtitle: [Construction de Systèmes Embarqués sur Linux],
  author: [Carolina Inácio Pereira, Gaby Roch, Simon Mirkovitch],
  department: "isis",
  report-no: [Rapport de laboratoire 1],
  open-right: true,
  abstract: none,
  blurb: [
    #set par(justify: true)
    #lorem(50)
  ],background: "circuit",
  date: datetime(year: 2025, month: 7, day: 17),
)

#set par(justify: true)

= Introduction

/* Structure du rapport
- En-tête
  - logos
  - établissement
  - titre
  - auteur
  - lien vers un dépôt git (github, gitlab, bitbucket, …)
  - lieu
  - date
  - version
- Entre 1 et 4 pages par thème (comme on traite un thème sur 2 semaines, ça fait entre 1 et 2 pages par semaine)
  - Résumé du laboratoire
  - Réponse aux questions
  - Synthèse sur ce qui a été appris/exercé
    - Non acquis
    - Acquis, mais à exercer
    - Parfaitement acquis
  - Remarques et choses à retenir
  - Feedback personnel sur le laboratoire
- Annexes (références/littérature, code source/patches dans fichier tar)
*/

= Environnement Linux embarqué



= Programmation noyau



