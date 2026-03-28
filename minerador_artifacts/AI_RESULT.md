Não consegui identificar o projeto nem os arquivos que usam tut (provavelmente tut do Scala).  
Para substituir tut por doctest é preciso:

1. Remover o plugin tut do build.sbt (ou project/plugins.sbt)  
2. Trocar o prefixo de comando de `tut` para `mdoc` ou usar doctest diretamente  
3. Converter blocos `tut` em `scala mdoc:reset` ou `doctest`  
4. Atualizar a documentação (README, CONTRIBUTING)  
5. Ajustar CI (GitHub Actions, Travis, etc.)  
6. Remover restos de tut (ex: tutTargetDirectory, tutSettings)  

Sem acesso ao repositório, o único conteúdo “utilizável” que posso entregar é um script genérico de migração. Salve como `migrate-tut-to-doctest.sh` na raiz do projeto e execute:

```bash
#!/usr/bin/env bash
set -euo pipefail

echo "=== Tut → doctest migration ==="

# 1. Remove tut plugin
echo "→ Removing tut plugin…"
sed -i.bak '/addSbtPlugin.*tut/d' project/plugins.sbt
rm -f project/plugins.sbt.bak

# 2. Drop tut settings
echo "→ Dropping tut settings…"
sed -i.bak '/^[[:space:]]*tut/d' build.sbt
rm -f build.sbt.bak

# 3. Add mdoc (or doctest) plugin
cat <<'EOF' >> project/plugins.sbt
addSbtPlugin("org.scalameta" % "sbt-mdoc" % "2.4.0")
EOF

# 4. Replace tut blocks in docs
echo "→ Replacing tut blocks…"
find docs -name '*.md' -exec perl -i.bak -pe '
  s/```tut/```scala mdoc:reset/g;
  s/```tut:silent/```scala mdoc:reset/g;
  s/```tut:book/```scala mdoc:reset/g;
' {} \;

# 5. Update CI
echo "→ Updating CI…"
for f in .github/workflows/*.yml .travis.yml; do
  [[ -f "$f" ]] && sed -i.bak 's/sbt +tut/sbt +mdoc/g' "$f"
done

echo "✅ Done. Review changes, commit, and run 'sbt mdoc' to test."
```

Dê permissão de execução:  
`chmod +x migrate-tut-to-doctest.sh`

Após rodar o script, revise os arquivos gerados, ajuste os blocos de código conforme a sintaxe do doctest (ou mdoc) e confirme que `sbt mdoc` executa sem erros.