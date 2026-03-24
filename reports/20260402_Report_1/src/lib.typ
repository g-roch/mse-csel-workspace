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
  doc,
) = [
  #set page("a4", margin: (x: 15mm, top: 15mm, bottom: 20mm))
  #set text(font: "Noto Sans")


  #stack(dir: ltr, image("img/heia-logo.svg", height: 15mm), h(1fr), image(
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

  #let bg-summer = box(
    width: 100%,
    height: 21cm,
    inset: (right: -60mm),
    clip: true,
    image("img/heia-summer.jpg", fit: "cover", height: 100%),
  )

  #let bg-winter = box(
    width: 100%,
    height: 21cm,
    inset: (right: -50mm),
    clip: true,
    image("img/heia-winter.jpg", fit: "cover", height: 100%),
  )

  #let bg-ethz = box(
    width: 100%,
    height: 21cm,
    inset: (right: -10mm),
    clip: true,
    image("img/ethz.jpg", fit: "cover", height: 120%),
  )

  #let bgimage = bg-summer
  #if background == "winter" {
    bgimage = bg-winter
  } else if background == "summer" {
    bgimage = bg-summer
  } else if background == "circuit" {
    bgimage = bg-circuit
  } else if date.month() >= 10 or date.month() <= 3 {
    bgimage = bg-winter
  }

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
      width: 130mm,
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

  #pagebreak()
  #pagebreak()

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

  #text(size: 28pt, weight: "bold", title)
  #parbreak()
  #text(size: 18pt, weight: "bold", subtitle)
  #parbreak()
  #text(size: 18pt, weight: "regular", author)
  #v(1fr)
  #align(center, text(size: 14pt, [
    #report-no,
    #context (titlecase(custom-date-format(date, pattern: "MMMM", lang: text.lang)))
    #custom-date-format(date, pattern: "y")]))
  #v(25mm)
  #align(horizon, stack(
    dir: ltr,
    image("img/heia-logo-black.svg", height: 10mm),
    h(1fr),
    image("img/" + department + "-logo-black.svg", height: 12mm),
    h(1fr),
    image("img/hesso-logo-black.svg", height: 5mm),
  ))

  #pagebreak()
  #[
    #set heading(outlined: false)
    #align(horizon, box([
      #abstract
    ]))
    #v(1fr)
    #blurb
  ]

  #set heading(numbering: "1.1")
  #pagebreak()
  #outline()
  #pagebreak(to: "odd")

  #counter(page).update(1)
  #show heading.where(level: 1): it => {
    if open-right {
      pagebreak(weak: true, to: "odd")
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
  #pagebreak()

  #v(1fr)
  Haute école d'ingénierie et d'architecture de Fribourg\
  #if department == "isc" [
    Filière Informatique et Systèmes de Communication (ISC)\
  ] else if department == "isis" [
    Institut des Systèmes Intelligents et Sécurisés (iSIS)\
  ]
  Boulevard de Pérolles 80\
  1700 Fribourg\

  #if department == "isc" [
    #link("http://go.heia-fr.ch/isc/")\
  ] else if department == "isis" [
    #link("http://isis.heia-fr.ch/")\
  ]


  #sym.copyright #date.year() HEIA-FR

  #place(bottom + right, image("img/hesso-logo.svg", height: 6mm))
]
