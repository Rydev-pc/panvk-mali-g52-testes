# DOCUMENTAÇÃO SIMPLES — Teste do driver PanVK (Mali) no seu celular

**Dispositivo:** Samsung SM-A065M · Android 16 · MediaTek Helio G85 (MT6768) · GPU Mali-G52 MC2
**Driver testado:** `mali-panvk-27.0.0-devel-r10.zip` → `libvulkan_panfrost.so` (18 MB, AArch64)
**Data:** 10/11/2026

---

## 1. O que é esse driver (em poucas palavras)

- É um **driver Vulkan real** (não placebo): substitui o driver gráfico da GPU Mali.
- Feito pela comunidade (JimVulkan) em cima do **Mesa** (código aberto da AMD/Intel/etc).
- O diferencial dele: conversa com o kernel do Android **sem root**, usando o driver
  proprietário da Arm (kbase, via `/dev/mali0`) — normal apps/jogos usam esse mesmo caminho.

## 2. Como testei

1. Extraí o `.so` do zip.
2. Carreguei ele **como se fosse um jogo**: chamei a interface padrão de driver Vulkan
   (`vk_icdGetInstanceProcAddr`) com um programa de teste (C e Python/ctypes).
3. Criei uma instância Vulkan, listei a GPU, li as propriedades e criei um "device".
4. Peguei o log interno do driver no **logcat** do Android.

## 3. Resultados ( tudo funcionando ✅ )

| Etapa | Resultado |
|---|---|
| Carregar a lib | ✅ |
| Criar instância Vulkan | ✅ (13 ms) |
| Achar a GPU | ✅ 1 GPU: **"PanVK-G52 MC2"** |
| Versão Vulkan | **1.4.363** (api `0x40416b`) |
| Vendor / ID | `0x13b5` (ARM) / `0x74021000` (gpu_id 0x7402 = G52, bate com seu chip) |
| Extensões | 157 (inclui as nativas do Android) |
| Fila gráfica | 1 família: GRÁFICO + COMPUTE + TRANSFER |
| Criar device | ✅ (10 ms) |
| Fim do teste | ✅ **exit=0, sem crash** |

O driver avisou: *"panvk is not a conformant Vulkan implementation, testing use only"*
(e *"not well-tested on v7"* — seu G52 é geração Bifrost v7, fora da rota principal).

## 4. Os "bugs" que achei… e o veredito (importante ler)

Durante os testes apareceu um **segfault**. Fui atrás com REA/Rizin (engenharia
reversa) e gdb, e a verdade foi engraçada:

1. **"Struct deslocado 12 bytes"** → **FALSA.** Era erro do MEU programa de teste
   (eu tinha definido o struct com um `reserved[3]` que não existe na spec atual do
   Vulkan). O struct do driver está 100% de acordo com a especificação.
2. **Segfault no teste em C** → **Também era meu.** O struct `VkPhysicalDeviceProperties`
   tem **832 bytes** e meu buffer tinha 640 → o driver escreveu os 832 e
   sobrescreveu memória do meu teste (stack overflow no harness, não no driver).
   Com buffer de 1024 bytes: **zero crash, exit=0**.
3. **Chain de `pNext` vazio** → **esse é real.** Quando um app pede
   `vkGetPhysicalDeviceProperties2` com structs extras na cadeia (ex: informações de
   driver), o fork preenche só o struct base e deixa o resto zerado. Efeitos práticos:
   geralmente nenhum para jogos simples; jogos que exigem essas infos podem
   comportar-se estranho (raro).
4. **Warning no fim do processo (segfault na saída)** → só aconteceu com buffers
   menores no meu harness; com o harness correto o processo encerrou limpo.

## 5. Detalhes internos que valem nota

- O driver expõe knobs de debug via propriedades `vendor.mesa.*`
  (ex.: `vendor.mesa.force.vk.devicename` — dá pra trocar o nome de GPU reportado,
  `vendor.mesa.pan.compute.core.mask` — limitar núcleos de compute, etc).
  No Termux apareceram "access denied" porque usuário normal não pode
  ler/definir essas propriedades — em contexto apropriado funcionam.
- Ele tem caminho duplo no código: `jm` (Job Manager, GPUs antigas) e `csf`
  (Command Stream Frontend, GPUs novas) — escolhe automaticamente.
- Suporte: **Bifrost e Valhall** (Mali-G31/G51/G52/G57/G68/G76/G77/G78/G710/G715/
  G720/G725 e Immortalis). **Midgard (Mali-4xx/T7xx/T8xx) NÃO suportado.**
- gralloc: usou fallback próprio (o gralloc da Samsung não suportou `lock_ycbcr`).

## 6. Entregáveis

- `panvk_test_log.txt` — log técnico do primeiro round de testes.
- `panvk_analise_detalhada.md` — este documento.
- `mali-panvk-27.0.0-devel-r10-validado.zip` — o driver ORIGINAL (sem nenhuma
  modificação binária) + relatório de validação + harness de teste em C.
- Harness de teste: `c_vktest.c` (compila no Termux com clang).

## 7. Conclusão

- O driver **não precisou de patch**: estava íntegro. As correções foram no método
  de teste (buffers e structs), e tudo passou limpo (exit=0).
- Único comportamento "estranho" real: cadeias `pNext` não preenchidas.
- No SEU chip (Bifrost v7 / G52): o driver roda, mas a própria aviso diz que essa
  geração é pouco testada — espere bugs em jogos pesados.

FIM
