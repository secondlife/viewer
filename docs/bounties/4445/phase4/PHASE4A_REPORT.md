# PHASE4A_REPORT

## Artefatos gerados
- Ferramenta: `tools/bounties/4445/phase4/gen_tut_to_doctest.py` (idempotente, cobre 41 arquivos TUT).
- Compatibilidade: `indra/test/tut_compat_doctest.h` com wrappers básicos (`TUT_ENSURE`, `TUT_REQUIRE`, etc.) e fallback de skip.
- Saída: 41 stubs em `indra/llcommon/tests_doctest` + `generated.index` documentando origens.

## Status de geração
| Situação | Quantidade | Observações |
| --- | --- | --- |
| Compila com TODO (`DOCTEST_FAIL`) | 41 | Todos os `_doctest.cpp`; aguardam migração manual. |

Detalhes por arquivo: consultar `indra/llcommon/tests_doctest/generated.index`.

## Exemplo (antes → stub gerado)
```
// llallocator_test.cpp
ensure_Equals("same size", alloc.getSize(), 64);

// llallocator_test_doctest.cpp
DOCTEST_FAIL("TODO: convert llallocator_test.cpp::object::test<1> from TUT to doctest");
// Original snippet:
// ensure_equals("same size", alloc.getSize(), 64);
```

## Como reproduzir
```powershell
# Gerar stubs
autobuild configure -c RelWithDebInfoOS -- -DLL_TESTS=ON
python tools\bounties\4445\phase4\gen_tut_to_doctest.py --src indra\llcommon\tests --dst indra\llcommon\tests_doctest

# Build do alvo
autobuild configure -c RelWithDebInfoOS -- -DLL_TESTS=ON
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build-vc170-64 --config RelWithDebInfo --target llcommon_doctest

# Execução (espera-se falhas TODO)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' -C RelWithDebInfo -R llcommon_doctest -V
```

## Saída relevante do ctest
```
[doctest] test cases: 372 | 0 passed | 372 failed | 0 skipped
[doctest] assertions: 372 | 0 passed | 372 failed |
```

## Riscos / próximos passos
- Necessidade de migrar cada stub substituindo `DOCTEST_FAIL` por asserts reais.
- Alguns includes platform-specific comentados pelo gerador; revisar conforme cada caso for migrado.
- `tut_compat_doctest.h` expõe apenas subset; ampliar conforme os stubs forem refinados.

Status: pipeline llcommon_doctest criado (build OK, ctest executa com TODOs). Próxima etapa (Fase 4B) = portar casos reais e remover DOCTEST_FAIL gradualmente.
