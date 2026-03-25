// SPDX-FileCopyrightText: 2025 Jacques Supcik <jacques.supcik@hefr.ch>
//
// SPDX-License-Identifier: MIT

#import "@preview/datify:1.0.0": *
#import "@preview/titleize:0.1.1": titlecase

#let heia_blue = rgb("#007CB7")
#let heia_grey = rgb("#ACA39A")

#let report(
  title: [Your Title Here],
  subtitle: none,
  author: [Author Name],
  department: "isc",
  abstract: none,
  report-no: "Report Number",
  background: none,
  date: none,
  blurb: none,
  open-right: false,
  repository: none,
  doc,
) = [
  #set page("a4", margin: (x: 15mm, top: 15mm, bottom: 20mm))
  #set text(font: "Noto Sans")


  #stack(dir: ltr, image("img/mse-small-margin.svg", height: 14mm), h(1fr), image(
    "img/" + department + "-logo.svg",
    height: 15mm,
  ))

  #let bg-circuit = box(
    width: 100%,
    height: 21cm,
    inset: (right: -60mm),
    clip: true,
    image("img/circuit.jpg", fit: "cover", height: 100%),
  )

  #let bgimage = bg-circuit

  #place(
    top + center,
    dy: 30mm,
    bgimage,
  )

  #place(
    top + left,
    dx: -10mm,
    dy: 50mm,
    box(
      width: 200mm,
      fill: heia_blue.darken(50%),
      inset: (x: 15mm, y: 15mm),

      align(horizon, stack(
        dir: ttb,
        par(leading: 1.4em, text(fill: white, weight: "bold", size: 28pt, [
          #title])),
        v(10mm),
        ..if (subtitle != none) {
          (
            text(fill: white, weight: "bold", size: 18pt, [#subtitle]),
            v(10mm),
          )
        },
        text(fill: white, weight: "regular", size: 18pt, [#author]),

        if (repository != none) {
          v(20mm)
          text(fill: white, weight: "regular", size: 16pt, [Dépôt Git : #repository])
        }
      )),
    ),
  )

  #place(
    bottom + center,
    stack(
      dir: ltr,
      text(
        size: 12pt,
        [#report-no,
          #context (titlecase(custom-date-format(date, pattern: "MMMM", lang: text.lang)))
          #custom-date-format(date, pattern: "y")],
      ),
      h(1fr),
      image(
        "img/hesso-logo.svg",
        height: 6mm,
      ),
    ),
  )

  // #pagebreak()
  // #pagebreak()

  #show heading: it => {
    text(fill: heia_blue, size: 1.3em, it)
  }
  #show heading.where(level: 1): it => {
    it
    v(.6em)
  }

  #counter(page).update(1)
  #set page(
    numbering: "1",
    footer: context {
      set text(fill: heia_grey)
      if calc.even(counter(page).get().first()) {
        align(left, counter(page).display("I"))
      } else {
        align(right, counter(page).display("I"))
      }
    },
  )
  #set page(margin: 25mm)
  
  #set heading(numbering: "1.1")
  #pagebreak()
  #outline()

  #counter(page).update(1)
  #show heading.where(level: 1): it => {
    if open-right {
      pagebreak()
    }
    it
  }
  #set page(
    numbering: "1",
    footer: context {
      if calc.even(counter(page).get().first()) {
        align(left, counter(page).display("1"))
      } else {
        align(right, counter(page).display("1"))
      }
    },
  )
  #doc
  #pagebreak(weak: true, to: "odd")
  #set page(
    numbering: none,
    footer: none,
  )
  #pagebreak()

  #v(1fr)
  Haute École Spécialisée de Suisse Occidentale\
  #if department == "hesso-master" [
    Master of Science in Engineering (MSE)\
  ] else [
    ""
  ]\
  Avenue de Provence 6\
  1007 Lausanne\

  #if department == "hesso-master" [
    #link("https://www.msengineering.ch/")\
  ]


  #sym.copyright #date.year() HES-SO Master

  #place(bottom + right, image("img/hesso-logo.svg", height: 6mm))
]
