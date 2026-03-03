# Notifications

## Anti-spam
### Only local notifications
- Problem: bot@spam.net could abuse our new notification endpoint.
- Solution: Only locally authenticated users/apps are allowed to send notifications.
- Workaround for remote 


## Possible fields
### Internal
- received timestamp: when the notification object was created
  - note: if the notification was received late, specify that in the description
- viewTs: if the user has seen this notification already
- notification id: locally unique id

### Set by creator 
- modId: module that created the notification
- user: user receiving the notification
- title: short notification message
- description: expanded notification message
- link: location to send the user if they click on the notification
- icon: relevant image for the notification

## Endpoints

### **POST** /create
Call this endpoint from a local mod.
- Body JSON object containing the following fields:
  - `title` (required): short notification message
  - `description`: expanded notification body
  - `link`: notification click action
  - `icon`: relevant image
  - `instance`: relevant instance
- The relevant mod id and user are set by the host
- Responses
  - Success: status of 200 with body set to the notification id
  - Errors: 400, 401, 500

### **GET** /list
Get a list of notifications for the user

- Query params:
  - after=<ts>: list notifications after given epoch ms
  - before=<ts>: list notifications before given epoch ms
    - useful for pagination
  - limit=N: respond with at most N results (default 25)
  - seen=1/0: 1 to only show seen notifications, 2 to only show unseen

### **GET** /count
Get the number of unread notifications for the user (ie - number to put on bell).

### **POST** /view/:id
View a notification.
- if :id is "all", set all user's notifications to viewed

### **POST** /delete/:id
Delete a notification
- if :id is "all", set all user's notifications to viewed

## Roadmap
### Stage 0
- new notification endpoint
- view notifications
- delete notifications

### Stage 1
- Filter Notifications
- Notification actions
- Archive Notifications (hidden but not deleted)
- View archived notifications
- Search notifications
  - By status
  - By instance

### Ideas
- Notification actions: ie - "delete", "reply", etc.
- Mobile app