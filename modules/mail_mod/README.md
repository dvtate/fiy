# Mail mod

## Endpoints
### **POST** /send
Handles sending and receiving of mail
- Body: json
  - to: recipient(s)
  - subject: subject
  - body: mail body

### **GET** /
View inbox

### **GET** /view/:item
View mail item