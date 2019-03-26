SELECT user_group AS user_group, user_friend AS user_friend, user_unread AS unreadMessage
FROM users.users
WHERE user_username = "843261040"
UNION ALL
SELECT friend_group AS user_group, user_username AS user_friend, friend_unread AS unreadMessage
FROM users.users
WHERE user_friend = "843261040";

SELECT user_unread AS unreadMessage
FROM users.users
WHERE user_username ="843261040"
UNION ALL
SELECT friend_unread AS unreadMessage
FROM users.users
WHERE user_friend ="843261040";


/*UPDATE users.users
SET user_unread = 3
WHERE user_username = "00000001" AND user_friend = "843261040";
UPDATE users.users
SET friend_unread = 3
WHERE user_username = "843261040" AND user_friend = "00000001";*/

/*UPDATE users.info
SET user_password = '00000000000', user_gender = '男'
WHERE user_username = '843261040';*/


INSERT INTO users.info 
VALUES("843261040", "00000000000", "MPS", "qrc:/image/head1.jpg", "qrc:/image/Background/7.jpg", "男", "1996-07-27", "嗷嗷嗷嗷嗷嗷嗷", 999);
INSERT INTO users.info 
VALUES("00000001", "00000000000", "mps1", "qrc:/image/HeadImage/head1.jpg", "qrc:/image/Background/7.jpg","男", "1995-01-23", "我不爱你了、", 37);
INSERT INTO users.info 
VALUES("00000002", "00000000000", "mps2", "qrc:/image/HeadImage/head2.jpg", "qrc:/image/Background/7.jpg","男", "1998-10-12", "哎 o(︶︿︶)o ", 16);
INSERT INTO users.info 
VALUES("00000003", "00000000000", "mps3", "qrc:/image/HeadImage/head3.jpg", "qrc:/image/Background/7.jpg","女", "1997-08-08", "累了、", 23);