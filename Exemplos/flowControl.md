<!-- ===== CONTROLE TEMPERATURA ===== -->

<!-- Q -->
temp_estavel = 10000 -> temp. estável
aumenta_temp_medio = 500000 -> aumenta médio
aumenta_temp_rapido = 700000 -> aumenta rápido

<!-- temp acima da referencia -->
se temp > temp_ref
  Q = temp_estavel

  se nivel - nivel_ref > 0.5   // nivel acima da ref+0.5


  se nivel - nivel_ref < -0.5  // nivel abaixo da ref-0.5


<!-- temp abaixo da referencia -->
se temp < temp_ref
  se (ref_temp - temp) * 20 < 0.5
    Q = (ref_temp - temp) * 20 + 10000
  senão
    Q = aumenta_temp_rapido


<!-- ===== CONTROLE NÍVEL ===== -->

<!-- Nf -->
nivel_estavel = 0.0 -> nivel. estável
diminui_nivel_medio = 60.0 -> diminui médio
diminui_nivel_rapido = 100.0 -> diminui rápido

<!-- nivel acima da referencia -->
se nivel > nivel_ref
  se (nivel_ref - nivel) * 20 < 0.35
    Nf = (nivel_ref - nivel) * 20
  senão
    Nf = diminui_nivel_rapido

<!-- nivel abaixo da referencia -->
se nivel < nivel_ref
  Nf = nivel_estavel

  <!-- aumenta nível com temp estável   -->
  se abs(temp - temp_ref) < 5



