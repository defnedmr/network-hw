# Proje Yol Haritası (Roadmap) ve Görev Dağılımı

Bu belge, TCP Tıkanıklık Kontrolü (Congestion Control) Algoritmaları Karşılaştırma ödevine ait proje yol haritasını ve iki kişilik görev dağılımını içermektedir. Proje temel olarak iki ana bölümden oluşmaktadır:
1. Altı (6) düğümlü (node) bir ağ topolojisi ve yönlendirme (routing) altyapısının kurulması.
2. Bu altyapı üzerinde çalışacak TCP Tıkanıklık Kontrolü algoritmasının (Tahoe, Reno veya NewReno) simüle edilmesi.

## 🗺️ Proje Yol Haritası (Roadmap)

### Aşama 1: Altyapı ve Topoloji Kurulumu
*   **Algoritma Seçimi:**   TCP Tahoe.
*   **Konfigürasyon ve Başlatma:** `A.conf` - `F.conf` dosyalarının C programı tarafından okunması (parsing).
*   **Ağ Topolojisinin Kurulması:** 6 farklı terminalde çalışacak düğümler (A,B,C,D,E,F) için Soket (Socket) bağlantılarının (Port 5001-5006) ayağa kaldırılması.
*   **Yönlendirme (Routing) İşlemi:** Yönlendirme tablolarının tanımlanması ve `send D hello...` komutlarında mesajın bir sonraki düğüm (next-hop) üzerinden hedefe iletilmesi (Forwarding).

### Aşama 2: TCP Veri Yapıları ve Temel Tıkanıklık Kontrolü
*   **Veri Yapıları (Structs):** Gönderici durumu, `cwnd` (congestion window), `ssthresh` (slow-start threshold), ACK sayaçları ve paket olayları için C `struct` yapılarının tanımlanması.
*   **Temel Durumlar (States):** Seçilen TCP algoritması için **Slow Start** ve **Congestion Avoidance** (Tıkanıklık Önleme) durumlarının kodlanması.
*   **Kayıp Simülasyonu:** Yapılandırılabilir (configurable) paket kaybı mekanizmasının geliştirilmesi.

### Aşama 3: Gelişmiş TCP Özellikleri ve Entegrasyon
*   **Hata Kurtarma:** Paket kaybı (Timeout) senaryolarının kodlanması. Üçlü Kopya ACK (Triple duplicate ACK) geldiğinde **Fast Retransmit** ve algoritmaya göre **Fast Recovery** (Hızlı Kurtarma) aşamalarının uygulanması.
*   **Sistem Entegrasyonu:** Geliştirilen TCP simülasyonunun, Aşama 1'de kurulan 6 düğümlü yönlendirme (routing) sistemine entegre edilmesi.
*   **Loglama:** Tıkanıklık penceresinin (`cwnd`) evriminin adım adım konsola yazdırılması.

### Aşama 4: Test, Analiz ve Teslimat
*   **Deneysel Analiz:** Farklı paket kaybı senaryoları altında sistemin test edilmesi, `cwnd` büyümesi ve veri aktarım hızı (throughput) performanslarının analiz edilmesi.
*   **Dokümantasyon:** Projenin nasıl derlenip çalıştırılacağını anlatan README dosyasının hazırlanması.
*   **Video Çekimi:** Tıkanıklık penceresi, timeout kaybı ve duplicate ACK senaryolarını gösteren, algoritmanın sözlü olarak anlatıldığı tanıtım videosunun çekilmesi.

---

## 👥 İki Kişi İçin Görev Dağılımı

Projenin yapısı göz önüne alındığında, iş yükünü **"Ağ/Yönlendirme (Network/Routing)"** ve **"TCP/Algoritma"** olmak üzere ikiye bölmek en verimli yöntemdir.

### 🧑‍💻 Kişi 1: Ağ ve Altyapı Sorumlusu (Network & System Focus)
*Bu kişi düğümlerin birbiriyle konuşmasını ve mesajların doğru rotadan gitmesini sağlamaktan sorumludur.*

*   **Görev 1:** 6 adet yapılandırma dosyasını (A.conf - F.conf) okuyacak C kodunu yazmak.
*   **Görev 2:** Soket programlama (TCP veya UDP) kullanarak 6 düğümün belirtilen portlardan haberleşme altyapısını kurmak.
*   **Görev 3:** Gereksinimlerdeki yönlendirme tablosunu (Routing Table) uygulamaya dökmek. Komut satırından girilen (`send D hello_from_A_to_D`) mesajı alıp işleyecek CLI mekanizmasını kurmak.
*   **Görev 4:** Mesajların ara düğümlerden ("next-hop") geçerek hedefe ulaşmasını sağlamak ve gereksinimlerdeki forwarding çıktılarını (Örn: `[B] Forwarding message from A to D...`) konsola yazdırmak.
*   **Görev 5:** Projenin README belgesini hazırlamak (Kurulum, çalıştırma talimatları vb.).

### 🧑‍💻 Kişi 2: TCP ve Algoritma Sorumlusu (TCP & Algorithm Focus)
*Bu kişi, mesaj iletimi sırasındaki TCP simülasyonunu, tıkanıklık penceresini ve veri paketleme kurallarını uygulamaktan sorumludur.*

*   **Görev 1:** Proje için gerekli Core Data Structure'ları (`cwnd`, `ssthresh`, packet state) C tarafında oluşturmak.
*   **Görev 2:** Gruba düşen TCP Algoritmasına göre (Tahoe, Reno veya NewReno) **Slow Start** ve **Congestion Avoidance** aşamalarını simüle etmek.
*   **Görev 3:** Timeout ve 3 Duplicate ACK gibi paket kaybı senaryolarını koda eklemek, kayıp durumunda çalışan **Fast Retransmit** ve **Fast Recovery** durumlarını kodlamak.
*   **Görev 4:** `cwnd` değerinin artış/azalış durumunu terminalde adım adım gösterecek loglama ekranını geliştirmek.
*   **Görev 5:** Deneysel analiz testlerini (throughput, kurtarma hızı) gerçekleştirmek ve sistemin tüm özelliklerini anlatan Teslimat (Demo) Videosunu kaydetmek.
