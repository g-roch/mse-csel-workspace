#import "@local/heiafr-report:0.1.0": report
#import "@preview/mitex:0.2.6": mitex
#import "@preview/cmarker:0.1.8"

#set text(lang: "en")

#show: report.with(
  title: [How to Write Elegant Code],
  subtitle: [A Practical Guide to Programming],
  author: [Jacques Supcik],
  department: "isis",
  report-no: [Technical Report 1],
  open-right: true,
  abstract: [
    = Abstract

    #lorem(29)

  ],
  blurb: [
    #set par(justify: true)
    #lorem(50)
  ],
  date: datetime(year: 2025, month: 7, day: 17),
)

#set par(justify: true)
= Introduction

#lorem(200)

== Test

#columns(2, lorem(200))

= New Chapter

#lorem(600)

