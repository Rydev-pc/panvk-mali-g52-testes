# Validação do driver PanVK (Mali) no Mali-G52 MC2 — Galaxy A06 (SM-A065M)

> Documentação simples e completa do teste de um driver Vulkan Mali real
> (PanVK) rodando no chip Mali-G52 MC2 (MediaTek Helio G85), no próprio
> celular, via Termux.

---

## O driver

- **Dono real / autor:** [JimVulkan/mali-panvk](https://github.com/JimVulkan/mali-panvk) (MIT)
- **Base:** Mesa 27.0.0-devel (git-58f2c55), com as modificações do JimVulkan
  para rodar em cima do **kbase** — o driver de kernel proprietário da Arm que
  os celulares já vêm com (`/dev/mali0`), **sem root e sem kernel custom**.
- Arquiteturas: **Bifrost (Job Manager)** e **Valhall/Gen10+ (CSF)**.
  Midgard (Mali-4xx/T7xx/T8xx) não é suportado.
- Zip testado (`r10`): `meta.json` packageVersion 11, `libvulkan_panfrost.so`
  de 18 MB (ELF AArch64, exportações ICD padrão), `NOTICE.txt`.

## Dispositivo de teste

**Samsung SM-A065M · Android 16 · Helio G85 (MT6768) · Mali-G52 MC2 (gpu_id 0x74021000) · kbase `/dev/mali0` crw-rw-rw- system:graphics**

## Como foi testado

Harness em C falando direto com a interface ICD padrão
(`vk_icdGetInstanceProcAddr`), compilado no Termux com clang:

```
clang -O0 c_vktest.c -o t -ldl
```

## Resultados

```
vk_icdNegotiateLoaderICDInterfaceVersion -> v0
vkCreateInstance                          -> VK_SUCCESS (~13 ms)
vkEnumeratePhysicalDevices                -> 1 GPU
  deviceName    = PanVK-G52 MC2
  apiVersion    = 1.4.363 (0x40416b)
  driverVersion = 0x06863063
  vendorID      = 0x13b5 (ARM)
  deviceID      = 0x74021000 (G52 ✓)
  extensões     = 157 (VK_ANDROID_native_buffer, AHB, ARM_shader_core_builtins, ...)
vkCreateDevice (queue graphics|compute|transfer) -> VK_SUCCESS (~10 ms)
Saída do processo: LIMPA (exit=0)
```

Avisos do próprio driver (normais, esperados):

```
WARNING: panvk is not a conformant Vulkan implementation, testing use only.
MESA W: panvk is not well-tested on v7 -- continuing anyway
```

## Falso bug x verdade (lição de engenharia reversa)

Durante o teste apareceu um segfault e uma "anomalia de layout" que passaram
por bugs do driver. Investigação com gdb/Rizin provou:

1. **"Struct deslocado 12 bytes"** → falso. Era o struct do MEU probe
   (`reserved[3]` não existe na spec atual do Vulkan; `deviceName` vem direto
   após `deviceType`, em @20). O driver está conforme o ABI.
2. **Segfault** → falso. `VkPhysicalDeviceProperties` tem **832 bytes**; buffers
   de 640/480/64 bytes do harness estouravam (stack/heap smash do harness,
   não do driver). Com buffer de 1024 bytes: **zero crash, exit=0**.
3. Log de depuração decisivo (gdb): crash em `ldr w2, [x21, 0x8c]` dentro do
   dispatcher (`0xe242a8`), chamado por `blr x8` em `main+672` do harness —
   a chamada errada era minha, com buffer pequeno demais.

**Achado real de comportamento:** cadeias `pNext` de
`vkGetPhysicalDeviceProperties2` não são preenchidas pelo fork (só o struct
base). Impacto prático: raro; jogos que exigem essas infos podem se
comportar estranho.

**Conclusão:** o driver não precisou de patch — rodou íntegro, sem nenhuma
modificação binária (md5 idêntico ao zip de origem).

## Detalhes internos

- Caminho duplo no código: `jm` (Job Manager, GPUs antigas) e `csf`
  (Command Stream Frontend, G710+), escolhido automaticamente.
- Knobs de debug via propriedades `vendor.mesa.*`
  (ex.: `vendor.mesa.force.vk.devicename`, `vendor.mesa.pan.compute.core.mask`).
- gralloc: usa fallback próprio (gralloc da Samsung não suportou `lock_ycbcr`).

## Ferramentas usadas

| Ferramenta | Uso neste projeto |
|---|---|
| **Termux** (Android) | Ambiente de execução de tudo, no próprio celular |
| **REA** ([morluto/rea](https://github.com/morluto/rea), `rea-agents` v6.8.0) | Toolkit de engenharia reversa (análise de binários) |
| **Rizin** 0.9.1 | Disassembly do `libvulkan_panfrost.so` (AArch64), localização do dispatcher `0xe242a8` |
| **GDB** 18.1 | Backtrace/rastreio do segfault, breakpoints com python API no gdb |
| **readelf / objdump** | ELF headers, program headers, dynamic symbols da ICD |
| **clang** (Termux) | Compilação do harness de teste em C |
| **Python 3 / ctypes** | Probes rápidos com structs Vulkan (instance/device/props) |
| **Vulkan-Headers** (Khronos) | Fonte da verdade do ABI (`vulkan_core.h`) |
| **zip / unzip** | Extração e empacotamento |

## Créditos e referências

- **JimVulkan** — autor do `mali-panvk` (o driver avaliado): https://github.com/JimVulkan/mali-panvk
  - Releases oficiais: https://github.com/JimVulkan/mali-panvk/releases
- **Mesa / Panfrost** — base do driver: https://www.mesa3d.org
- **Khronos Group** — especificação Vulkan / Vulkan-Headers: https://github.com/KhronosGroup/Vulkan-Headers
- **morluto/rea** — ferramenta de engenharia reversa usada: https://github.com/morluto/rea
- Ecossistema relacionado (pesquisa): zenithblue-oss/panvk-kbase-android, whaltermc/panvk,
  wonderkast02/panvk-g720-kbase-csf, BossDrk/0x8055 (pesquisa G52).

> **Nota:** este repositório documenta testes e análise. NÃO redistribui o binário
> do driver — baixe sempre da fonte oficial (JimVulkan/mali-panvk). Este projeto não
> é afiliado à Arm, Samsung, MediaTek, Google ou Mesa.

## Arquivos

- `README.md` — este documento
- `docs/panvk_analise_detalhada.md` — passo a passo simples da análise do driver
- `docs/panvk_test_log.txt` — log técnico do primeiro round de testes
- `tools/RELATORIO-VALIDACAO.txt` — relatório final de validação
- `tools/c_vktest.c` — harness de teste em C (compile no Termux: `clang -O0 c_vktest.c -o t -ldl`)
