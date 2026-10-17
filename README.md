# rc-wow
custom RC Car Radio, with a LR1121 for dual band access!
## project overview
this project contains a LR1121, ESP32, a microSD card, a USBC receptical, a spot for an OLED and a spot to add in a radio antenna, as well as other features to switch between frequencies and control power so it works both battery powered and when plugged in.
## pictures
<img width="503" height="628" alt="Screenshot 2026-10-11 at 3 39 02 PM" src="https://github.com/user-attachments/assets/dade35c8-bc6a-4e18-a7ba-d620e84d15d0" />
<img width="541" height="712" alt="Screenshot 2026-10-11 at 3 38 50 PM" src="https://github.com/user-attachments/assets/174747d2-55b4-4f43-a2e2-d8e47dc430bf" />
<img width="1256" height="480" alt="Screenshot 2026-10-11 at 3 38 33 PM" src="https://github.com/user-attachments/assets/a62a6ea4-1683-4d16-918c-98b492aab4be" />
<img width="1089" height="729" alt="Screenshot 2026-10-11 at 3 38 23 PM" src="https://github.com/user-attachments/assets/864fdd39-2c9a-49f3-88ac-9fbf5a363d7c" />
<img width="341" height="729" alt="Screenshot 2026-10-11 at 3 38 09 PM" src="https://github.com/user-attachments/assets/c6e39acd-50fe-4dc1-9ecd-0e02c6dcc3eb" />
## Bill of Materials (BOM)

| Part | Purpose | Qty | Unit Price | Total | Vendor |
|---|---|---:|---:|---:|---|
| [Battery Pack](https://www.aliexpress.us/item/3256807960114360.html) | Battery | 1 | $0.50 | $0.50 | AliExpress |
| [SD Card Reader](https://www.mouser.com/en/ProductDetail/Wurth-Elektronik/693072010801) | SD card reader | 1 | $3.50 | $3.50 | Mouser |
| [USB-C Receptacle](https://www.digikey.com/en/products/detail/gct/USB4085-GF-A/9859662) | USB connection | 1 | $1.00 | $1.00 | DigiKey |
| [Schottky Diode](https://www.aliexpress.us/item/3256808944462819.html) | Signal clearance | 1 | $3.50 | $3.50 | AliExpress |
| [Ferrite Bead](https://www.digikey.com/en/products/detail/laird-signal-integrity-products/28C0236-0JW-10/806596) | Ferrite bead | 1 | $1.10 | $1.10 | DigiKey |
| [0900FM15K0039001E](https://www.digikey.com/en/products/detail/johanson-technology-inc/0900FM15K0039001E/16639329) | Frequency switching | 2 | $0.50 | $1.00 | DigiKey |
| [0900DP15A2450001E](https://www.digikey.com/en/products/detail/johanson-technology-inc/0900DP15A2450001E/7943531) | Frequency switching | 1 | $1.00 | $1.00 | DigiKey |
| [ESP32-S3-WROOM-1-N8R8](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-S3-WROOM-1-N8R8/15295891) | Main controller | 1 | $7.00 | $7.00 | DigiKey |
| [PE4259](https://www.digikey.com/en/products/detail/psemi/4259-63/2614473) | Frequency switching | 3 | $1.00 | $3.00 | DigiKey |
| [XC6220B331MR-G](https://www.digikey.com/en/products/detail/torex-semiconductor-ltd/XC6220B331MR-G/2138177) | Power regulation | 1 | $1.00 | $1.00 | DigiKey |
| [LR1121IMLTRT](https://www.digikey.com/en/products/detail/semtech-corporation/LR1121IMLTRT/17874420) | Frequency switching | 1 | $8.50 | $8.50 | DigiKey |
| [PMOS](https://www.digikey.com/ordering/shoppingcart) | Switching | 1 | $1.00 | $1.00 | DigiKey |
| [Crystals](https://www.digikey.com/en/products/detail/ecs-inc/ECS-320-8-36B-CKY-TR/9838841) | Oscillator | 2 | $1.00 | $2.00 | DigiKey |
| [OLED Display](https://www.aliexpress.us/item/3256812744946971.html) | Display | 1 | $3.00 | $3.00 | AliExpress |
| [Coaxial Cable Connector](https://www.digikey.com/en/products/detail/joymax-electronics/CT-SAD010XM/27545740) | Antenna connector | 1 | $2.50 | $2.50 | DigiKey |
| [Dual-Band Antenna](https://radiomasterrc.com/products/dual-band-2-4ghz-sub-g-900mhz-antenna) | Antenna | 1 | $15.00 | $15.00 | RadioMaster |
| [PCB](https://jlcpcb.com) | PCB for all components | 1 | $13.00 | $13.00 | JLCPCB |
| **Parts Subtotal** | — | — | — | **$67.60** | — |
| **Tax & Shipping** | — | — | — | **$0.00** | — |
| **Grand Total** | — | — | — | **$67.60** | — |
## notes
this should hopefully work! i had to calculate impedance for the RF traces and such, and it should work fine. however, updates may be pushed and future versions may be made :D also i have a HAM radio license so double check that things are legal on your end.
