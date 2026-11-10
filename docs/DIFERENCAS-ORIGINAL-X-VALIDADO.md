# Diferenças: ORIGINAL x PACOTE VALIDADO

## Resumo em uma frase

**O binário do driver é IDÊNTICO ao original — nada foi alterado nele.**
Todas as "correções" foram no **método de teste** (harness C e structs),
não no driver.

## Prova (checksums)

`libvulkan_panfrost.so`:

```
ORIGINAL (extraído do zip mali-panvk-27.0.0-devel-r10.zip):
  md5    = 5d633f4d5c4533a80ac9cb8d493a98b3
  size   = 18764056 bytes

PACOTE VALIDADO (release/ deste repo):
  md5    = 5d633f4d5c4533a80ac9cb8d493a98b3   ← IDÊNTICO
  size   = 18764056 bytes
```

`meta.json`, `NOTICE.txt`: byte a byte iguais aos do zip de origem.

## O que o pacote validado contém a mais

| Arquivo | Origem |
|---|---|
| `libvulkan_panfrost.so` | original, intocado |
| `meta.json` | original, intocado |
| `NOTICE.txt` | original, intocado |
| `RELATORIO-VALIDACAO.txt` | **novo** — relatório dos testes executados no SM-A065M |
| `harness_teste.c` | **novo** — código do programa de teste usado |

## As diferenças de verdade (todas no harness de teste)

O segfault e a "anomalia de 12 bytes" do primeiro round de testes eram bugs
do harness, não do driver. Correções aplicadas no harness:

| # | Problema do harness antigo | Correção |
|---|---|---|
| 1 | Buffer de `VkPhysicalDeviceProperties` com 640/480/64 bytes (struct real tem **832 bytes**) → o driver escrevia os 832 e estourava a memória do harness (stack/heap smash) | Buffer de **1024 bytes** ≥ 832 → zero crash, exit=0 |
| 2 | Struct do probe declarado com `reserved[3]` **inexistente** na spec atual do Vulkan → parecia que o driver preenchia "deslocado 12 bytes" | Struct conforme o `vulkan_core.h` oficial (Khronos): `deviceName` em @20, `pipelineCacheUUID` em @276, `limits` em @292, total 832 bytes |
| 3 | Segfault no teardown (lixo `0x500000005` = ponteiro corrompido pelo overflow do item 1) | Desapareceu junto com a correção do buffer |
| 4 | Chain `pNext` testada com struct subdimensionado | Struct correto (152 bytes) — resultado: o fork **realmente** não preenche chains de `Properties2` (único achado legítimo) |

## Por que não "corrigi" o driver

Não havia nada a corrigir: rodou íntegro no Mali-G52 MC2
(instance/enumerate/props/vkCreateDevice todos `VK_SUCCESS`, saída limpa).
Qualquer patch no binário sem motivo real seria pior (quebraria os checksums
e a rastreabilidade com a fonte oficial do JimVulkan).

**Baixe sempre a versão mais recente da fonte oficial:**
https://github.com/JimVulkan/mali-panvk/releases
