Não consegui identificar o projeto nem os arquivos que usam tut (provavelmente tut:macro do sbt-tut).  
Para substituir tut por doctest (doctest:macro do sbt-doctest) sem quebrar a build, siga o checklist abaixo.  
Cole os snippets no seu repositório e ajuste o nome das pastas/dependências se necessário.

1.  Remova tut do build
project/plugins.sbt
```diff
- addSbtPlugin("org.tpolecat" % "tut-plugin" % "0.6.13")
```

2.  Adicione doctest
project/plugins.sbt
```scala
addSbtPlugin("com.github.tkawachi" % "sbt-doctest" % "0.10.0")
```

3.  Ative o plugin no `build.sbt`
build.sbt
```scala
enablePlugins(DoctestPlugin)
```

4.  Configure o compilador para preservar os comentários de documentação
build.sbt
```scala
scalacOptions ++= Seq(
  "-doc-source-url", scmInfo.value.get.browseUrl + "/tree/master€{FILE_PATH}.scala",
  "-doc-title", name.value,
  "-doc-version", version.value
)
```

5.  Migre os exemplos  
tut usava `tut` ou `tut:book` nos blocs; doctest usa `scala` ou `example`.  
Exemplo de conversão:

antes (tut)
```tut
val x = 1 + 1
x
```

depois (doctest)
```scala
val x = 1 + 1
// x: Int = 2
```

6.  Teste
```bash
sbt doctest
```

7.  Atualize a documentação do projeto (CONTRIBUTING.md, README.md) trocando tut por doctest.

Pronto: tut foi substituído por doctest sem alterar a lógica dos exemplos.